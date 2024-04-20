#include "server/Server.hpp"

#include <boost/asio/io_context.hpp>

#include "Common.hpp"
#include "Constants.hpp"
#include "Types.hpp"
#include "game/Actor.hpp"
#include "game/Game.hpp"
#include "game/Scene.hpp"
#include "net/Host.hpp"
#include "net/Messages.hpp"
#include "net/Replicator.hpp"
#include "resources/Configs.hpp"
#include "scripting/Libs.hpp"
#include "scripting/Scripting.hpp"
#include "server/ServerInterface.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#ifdef TRACK_FPS
#    include "util/FPS.hpp"
#endif

namespace sge {

namespace server {

using std::chrono::high_resolution_clock;
using std::chrono::time_point;

namespace {

void sleepToTargetTickRate(const time_point<high_resolution_clock> &tickStart,
                           std::chrono::microseconds targetTickTime) {
    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    using std::chrono::milliseconds;

    time_point<high_resolution_clock> now;
    microseconds delta;

    now = high_resolution_clock::now();
    delta = duration_cast<microseconds>(now - tickStart);
    while (delta < std::max(targetTickTime, microseconds(1000))) {
        std::this_thread::sleep_for(microseconds(500));
        now = high_resolution_clock::now();
        delta = duration_cast<microseconds>(now - tickStart);
    }
}

} // namespace

Server::Server(resources::ServerConfig serverConfig, resources::GameConfig gameConfig,
               boost::asio::io_context &ioContext)
    : serverConfig_(std::move(serverConfig))
    , gameConfig_(std::move(gameConfig))
    , host_(net::Host::create(ioContext, this->serverConfig_.port)) {
    scripting::Initialize();
    scripting::InitializeInterface(std::make_unique<ServerInterface>());

    if (this->serverConfig_.tick_rate > 100) {
        std::cerr << "Error: server tick_rate of " << this->serverConfig_.tick_rate
                  << " is too high (limit: 100)" << std::endl;
        std::exit(0);
    }

    // Initialize game and load initial scene
    this->initGame();

    // Start hosting server
    this->host_->start();
}

void Server::run() {
    assert(this->game_ != nullptr);

    auto targetTickTime = std::chrono::microseconds(1000000) / this->serverConfig_.tick_rate;
    time_point<high_resolution_clock> tickStart;

    while (this->running_) {
#ifdef TRACK_FPS
        util::StartFrame();
#endif

        tickStart = high_resolution_clock::now();
        this->tick();
        sleepToTargetTickRate(tickStart, targetTickTime);

#ifdef TRACK_FPS
        util::EndFrame();
#endif
    }
}

net::ReplicatorService &Server::replicatorService() {
    return this->replicatorService_;
}

unsigned int Server::tickNum() const {
    return this->tickNum_;
}

std::vector<client_id_t> Server::joinedClients() const {
    // Find all joined clients
    std::vector<client_id_t> joinedClients;
    for (const auto &state : this->clientStates_) {
        if (state.second == ClientState::Joined) {
            joinedClients.push_back(state.first);
        }
    }
    return joinedClients;
}

void Server::tick() {
    // 1. Process network events and messages. This includes client join/leave
    // events and all other general messages like MessageHello.
    this->processNetwork();

    // If there are no clients connected, conditionally proceed based on server
    // configuration's empty_behavior value.
    if (this->clientStates_.empty() &&
        this->serverConfig_.empty_behavior != resources::ServerEmptyBehavior::Run) {
        // Behavior is either pause or reset, either way don't update.
        return;
    }

    // 2. Run the game update loop. Executes OnStart, OnUpdate, etc.
    this->updateGame();

    // 3. Gather any replication requests that were created during the previous
    // update loop and broadcast them to all joined clients.
    this->executeReplications();

    // 4. Execute any deferred actions.
    this->executeAfterUpdates();

    // 5. Increment the tick counter. Note that if the game is paused due to no
    // connected clients, the tick counter will not increase.
    ++this->tickNum_;
}

void Server::initGame() {
    // Create game instance
    this->game_ = std::make_unique<game::Game>(this->gameConfig_);
    this->game_->loadScene(this->serverConfig_.initial_scene);
}

void Server::updateGame() {
    // Check for going to another scene
    if (!this->nextScene_.empty()) {
        this->swapScene(this->nextScene_);
        this->nextScene_.clear();
    }

    // Check for sending out room state updates
    if (this->roomStateChanged_) {
        this->broadcastRoomState();
        this->roomStateChanged_ = false;
    }

    // Run the game update logic
    this->game_->update();
}

void Server::swapScene(const std::string &name) {
    // Increment the generation counter ahead of the scene loading
    ++this->generation_;

    // Clear any pending replications
    this->replicatorService_.clear();

    // Switch the scene
    this->game_->loadScene(name);

    // Inform all connected clients about the new scene
    this->host_->broadcastMessage(net::MessageLoadScene{
        .generation = this->generation_,
        .sceneName = name,
        .runtimeActors = {}, // No updates yet = no runtime actors
        .sceneState = {},    // Fresh scene, no additional state to replicate
    });
}

void Server::setNextScene(std::string_view name) {
    this->nextScene_ = name;
}

void Server::clientJoined(client_id_t clientID) {
    // Update client state map
    assert(this->clientStates_.contains(clientID));
    this->clientStates_[clientID] = ClientState::Joined;

    // Mark server as needing to send out a room state update this tick
    this->roomStateChanged_ = true;

    this->doAfterUpdate([this, clientID] {
        // Process any subscriptions to client join events
        this->game_->eventSub().publish(events::MultiplayerOnClientJoin, clientID);
    });
}

void Server::clientLeft(client_id_t clientID) {
    // Erase client from state map
    this->clientStates_.erase(clientID);

    // Destroy any actors owned by the client that left
    for (auto &actor : this->game_->currentScene().actors()) {
        if (actor->ownerClient == clientID) {
            actor->destroy();
        }
    }

    if (this->clientStates_.empty() &&
        this->serverConfig_.empty_behavior == resources::ServerEmptyBehavior::Reset) {
        // There are no connected clients, we are supposed to reset the game now.
        this->initGame();
        // Because there are no connected clients, we also don't need to send out
        // room state updates (there are no clients connected to receive them!).
        // We also just initialized a new game so it doesn't make sense to publish
        // any events::MultiplayerOnClientLeave events, so just return.
        return;
    }

    // Mark server as needing to send out a room state update this tick
    this->roomStateChanged_ = true;

    this->doAfterUpdate([this, clientID] {
        // Process any subscriptions to client leave events
        this->game_->eventSub().publish(events::MultiplayerOnClientLeave, clientID);
    });
}

void Server::processNetwork() {
    // 1. Process "ClientEvent" events. These are socket/connection level events
    // like connect/disconnect.
    this->host_->consumeAllClientEvents([&](std::unique_ptr<net::ClientEvent> event) {
        switch (event->event) {
        case net::ClientEventType::Connected:
            this->clientStates_.emplace(event->clientID, ClientState::Initializing);
            break;
        case net::ClientEventType::Disconnected:
            this->clientLeft(event->clientID);
            break;
        }
    });

    // 2. Process messages from clients. Execute all required handling logic and
    // send all required messages/broadcasts as a result of executing the handlers.
    this->host_->consumeAllClientMessages([&](std::unique_ptr<net::ClientMessage> msg) {
        this->processMessage(std::move(msg));
    });
}

void Server::executeReplications() {
    this->executeTickReplication();
    this->executeRemoteEvents();
}

void Server::executeTickReplication() {
    if (!this->replicatorService_.hasPendingReplications()) {
        // No replication data to send
        return;
    }

    // Server-side actor instantiations or state replications have occurred.
    // Serialize them and broadcast them to all joined clients.
    auto instantiations = this->replicatorService_.serializeInstantiations();
    auto replicationRequests = this->replicatorService_.serializeComponents();
    auto destructions = this->replicatorService_.serializeDestructions();
    assert(!instantiations.empty() || !replicationRequests.empty() || !destructions.empty());

    this->broadcastToJoined(net::MessageTickReplication{
        .generation = this->generation_,
        .instantiations = std::move(instantiations),
        .replications = std::move(replicationRequests),
        .destructions = std::move(destructions),
    });
}

void Server::executeRemoteEvents() {
    if (!this->replicatorService_.hasPendingEventPublishes()) {
        // No remote events to send
        return;
    }

    auto publishes = this->replicatorService_.serializeEventPublishes();
    assert(!publishes.empty());

    this->broadcastToJoined(net::MessageRemoteEvents{
        .generation = this->generation_,
        .publishes = std::move(publishes),
    });
}

//-----------------------------------------------------------------------------
// Client message processing

void Server::processMessage(std::unique_ptr<net::ClientMessage> msg) {
    if (!this->clientStates_.contains(msg->clientID)) {
        std::cerr << "warning: dropping message from client " << msg->clientID
                  << " because it appears to no longer be connected" << std::endl;
        return;
    };
    std::visit(
        [this, cid = msg->clientID](auto &m) {
            this->processMessage(cid, m);
        },
        *msg->msg);
}

void Server::processMessage(client_id_t clientID, const net::MessageError &m) {
    std::cerr << "[ MSG_ERROR ] Client " << clientID << ": " << m.error << std::endl;
    this->host_->disconnectClient(clientID);
}

void Server::processMessage(client_id_t clientID, const net::MessageHello &m) {
    // Mark client ID as joined to game
    this->clientJoined(clientID);

    // Replicate any actors created at runtime
    auto runtimeActors = net::ReplicatorService::replicateRuntimeActors(*this->game_);
    // Replicate entire game state in form of ReplicationRequests
    auto sceneState = this->replicatorService_.replicateGame(*this->game_);

    // Send MessageWelcome with assigned client ID and tick rate
    this->host_->postMessage(clientID,
                             net::MessageWelcome{
                                 .clientID = clientID,
                                 .serverTickRate = this->serverConfig_.tick_rate,
                             });

    // Send current scene state
    this->host_->postMessage(clientID,
                             net::MessageLoadScene{
                                 .generation = this->generation_,
                                 .sceneName = this->game_->currentScene().name(),
                                 .runtimeActors = std::move(runtimeActors),
                                 .sceneState = std::move(sceneState),
                             });
}

void Server::processMessage(client_id_t clientID, const net::MessageLoadSceneRequest &m) {
    // A client wants to swap to another scene. Check to make sure the
    // generation number matches before confirming the switch.
    if (m.generation != this->generation_) {
        // Silently drop the request
        return;
    }
    // Set the next scene to load at the beginning of the game update
    // phase. This means that if another client comes along and also
    // requests to load a different scene, the later client will win.
    this->setNextScene(m.sceneName);
}

void Server::processMessage(client_id_t clientID, net::MessageTickReplication &m) {
    // A client has updated the game state and has requested to replicate
    // the state to the server and other clients.
    // 1. Instantiate any new actors.
    // 2. Process any replication requests.
    // 3. Acknowledge the tick replication to the sender client.
    // 4. Broadcast the replication to all other clients.
    auto &scene = this->game_->currentScene();

    if (m.generation != this->generation_) {
        // Mismatched generation. The server has loaded a new scene but
        // the client has not yet processed the new load. Reject any actor
        // instantiations and let the client know so they can appropriately
        // keep the game in sync.
        if (!m.instantiations.empty()) {
            std::vector<actor_id_t> rejectedInstantiations;
            rejectedInstantiations.reserve(m.instantiations.size());
            for (const auto &instantiation : m.instantiations) {
                rejectedInstantiations.push_back(instantiation.id);
            }
            this->host_->postMessage(
                clientID,
                net::MessageTickReplicationReject{
                    .serverGeneration = this->generation_,
                    .rejectedInstantiations = std::move(rejectedInstantiations),
                });
        }
        // Drop the replication message -- the state to replicate no
        // longer exists as we are in a different scene.
        return;
    }

    // Clients shouldn't send tick replications for no reason, but avoid
    // doing work that we don't need to do
    if (m.instantiations.empty() && m.replications.empty() && m.destructions.empty()) {
        return;
    }

    // Process all actor instantiations
    std::vector<net::RemoteIDMapping> remoteIDMappings;
    std::vector<net::InstantiatedActor> rewrittenInstantiations;
    remoteIDMappings.reserve(m.instantiations.size());
    rewrittenInstantiations.reserve(m.instantiations.size());
    for (auto &instantiation : m.instantiations) {
        // Instantiate the runtime actor
        auto* a = scene.instantiateRuntimeActor(instantiation.actorTemplate, instantiation.owner);
        // Map client-side id to server-side id
        remoteIDMappings.emplace_back(instantiation.id, a->id);
        // Reconstruct the instantiation with server-side actor id so
        // we can broadcast to other clients.
        rewrittenInstantiations.emplace_back(instantiation.actorTemplate,
                                             a->id,
                                             a->ownerClient,
                                             std::move(instantiation.componentState));
    }

    // Process all replication requests, updating the game state.
    // Note that the client should only be replicating components from
    // actors who are intrinsic to the scene or have been ack'd.
    for (const auto &req : m.replications) {
        this->processReplicationRequest(req);
    }

    // Process all actor destructions
    for (auto id : m.destructions) {
        // Find the actor we want to destroy
        auto* a = scene.findActorByID(id);
        if (a == nullptr) {
            continue;
        }
        // Found it, so destroy it. Don't replicate it to clients later
        // because we are going to do so below.
        a->destroyLocally();
    }

    // Acknowledge tick replication if required
    if (!remoteIDMappings.empty()) {
        this->host_->postMessage(clientID,
                                 net::MessageTickReplicationAck{
                                     .remoteIDMappings = std::move(remoteIDMappings),
                                 });
    }

    // Broadcast replication message to all other connected clients
    this->broadcastToOthers(
        net::MessageTickReplication{
            .generation = this->generation_,
            .instantiations = std::move(rewrittenInstantiations),
            .replications = std::move(m.replications),
            .destructions = std::move(m.destructions),
        },
        clientID);
}

void Server::processMessage(client_id_t clientID, net::MessageRemoteEvents &m) {
    // The client has some remote events for us.
    // 1. Verify the generation number
    // 2. Forward the remote events to all other clients
    // 3. Dispatch the published events to the server's EventSub instance

    if (m.generation != this->generation_) {
        // Silently drop the request
        return;
    }

    // Clients shouldn't send empty publishes but check so we don't end up
    // doing any unneeded work.
    if (m.publishes.empty()) {
        return;
    }

    // Forward this publish to other clients
    this->broadcastToOthers(m, clientID);

    this->doAfterUpdate([this, publishes = std::move(m.publishes)] {
        // Dispatch to game EventSub
        for (const auto &p : publishes) {
            this->game_->eventSub().publish(p.event, p.value);
        }
    });
}

//-----------------------------------------------------------------------------

void Server::processReplicationRequest(const net::ComponentReplication &replication) {
    // No interp on server
    net::ReplicatorService::dispatchReplication(*this->game_, replication, false);
}

void Server::sendInvalidMessage(client_id_t clientID) {
    this->host_->postMessage(clientID,
                             net::MessageError{
                                 .error = "invalid message received",
                             });
    this->host_->disconnectClient(clientID);
}

void Server::broadcastToJoined(const net::SMessage &msg) {
    if (this->clientStates_.empty()) {
        // No clients to broadcast to
        return;
    }
    this->host_->broadcastMessage(msg, [&](client_id_t cid) {
        return this->isJoined(cid);
    });
}

void Server::broadcastToOthers(const net::SMessage &msg, client_id_t src) {
    assert(this->isJoined(src));
    if (this->clientStates_.size() <= 1) {
        // No other clients to broadcast to
        return;
    }
    this->host_->broadcastMessage(msg, [&](client_id_t cid) {
        return cid != src && this->isJoined(cid);
    });
}

void Server::broadcastRoomState() {
    // Broadcast the joined clients to all clients
    this->broadcastToJoined(net::MessageRoomState{
        .joinedClients = this->joinedClients(),
    });
}

bool Server::isJoined(client_id_t clientID) const {
    auto it = this->clientStates_.find(clientID);
    if (it == this->clientStates_.end()) {
        return false;
    }
    return it->second == ClientState::Joined;
}

void Server::doAfterUpdate(std::function<void()> f) {
    this->afterUpdate_.emplace_back(std::move(f));
}

void Server::executeAfterUpdates() {
    for (const auto &after : this->afterUpdate_) {
        std::invoke(after);
    }
    this->afterUpdate_.clear();
}

std::unique_ptr<Server> EngineServer = nullptr;

void InitServer(resources::ServerConfig serverConfig, resources::GameConfig gameConfig,
                boost::asio::io_context &ioContext) {
    EngineServer =
        std::make_unique<Server>(std::move(serverConfig), std::move(gameConfig), ioContext);
}

void DeinitServer() {
    EngineServer.reset();
}

Server &CurrentServer() {
    assert(EngineServer != nullptr);
    return *EngineServer;
}

} // namespace server

game::Game &CurrentGame() {
    return *server::CurrentServer().game_;
}

game::Scene &CurrentScene() {
    return CurrentGame().currentScene();
}

bool GameOffline() {
    return false;
}

} // namespace sge
