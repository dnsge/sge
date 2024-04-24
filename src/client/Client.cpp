#include "client/Client.hpp"

#include <SDL2/SDL.h>
#include <boost/asio/io_context.hpp>

#include "Common.hpp"
#include "Constants.hpp"
#include "Renderer.hpp"
#include "Types.hpp"
#include "client/ClientInterface.hpp"
#include "game/Game.hpp"
#include "game/Input.hpp"
#include "game/Scene.hpp"
#include "net/Client.hpp"
#include "net/Messages.hpp"
#include "net/Replicator.hpp"
#include "render/Text.hpp"
#include "resources/Configs.hpp"
#include "scripting/Libs.hpp"
#include "scripting/Scripting.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#ifdef TRACK_FPS
#    include "util/FPS.hpp"
#endif

namespace sge {

namespace client {

Client::Client(resources::ClientConfig clientConfig, resources::GameConfig gameConfig,
               boost::asio::io_context &ioContext)
    : clientConfig_(std::move(clientConfig))
    , gameConfig_(std::move(gameConfig))
    , netClient_(ioContext) {
    scripting::Initialize();
    scripting::InitializeInterface(std::make_unique<ClientInterface>());

    Renderer::initialize(this->gameConfig_.window_title,
                         this->clientConfig_.rendering_config,
                         this->gameConfig_.font);
    game::Input::init();

    // Create game instance
    this->game_ = std::make_unique<game::Game>(this->gameConfig_);
    this->game_->loadScene(this->clientConfig_.initial_scene);

    // Register internal event handlers
    this->registerInternalEvents();
}

void Client::run() {
    while (this->running_) {
#ifdef TRACK_FPS
        util::StartFrame();
#endif

        this->readInput();
        this->update();
        this->render();

#ifdef TRACK_FPS
        util::EndFrame();
#endif

        // present also calls SDL_Wait as required
        Renderer::present();
    }
}

void Client::destroy() {
    this->game_->destroy();
}

const resources::ClientConfig &Client::config() const {
    return this->clientConfig_;
}

net::Client &Client::netClient() {
    return this->netClient_;
}

net::ReplicatorService &Client::replicatorService() {
    return this->replicatorService_;
}

bool Client::gameOffline() const {
    return this->state_ == State::Offline;
}

client_id_t Client::clientID() const {
    return this->clientID_;
}

std::vector<client_id_t> Client::joinedClients() const {
    return std::vector<client_id_t>{this->roomState_.begin(), this->roomState_.end()};
}

void Client::readInput() {
    bool quit = game::Input::loadPendingEvents();
    if (quit) {
        this->running_ = false;
    }
}

void Client::update() {
    // 1. If we are connected to a server, process network messages from the server.
    if (this->state_ != State::Offline) {
        this->processNetwork();
    }

    // 2. Check if we want to go to a new scene. If so, send the request to the server.
    if (!this->nextScene_.empty()) {
        this->requestSceneSwap();
        this->nextScene_.clear();
    }

    // 3. Run the game update loop.
    this->updateGame();

    if (this->state_ != State::Offline) {
        // 4. Gather any replication requests that were created during the previous
        // update loop and send them to the server.
        this->executeReplications();

        // 5. Execute any deferred actions.
        this->executeAfterUpdates();
    }
}

void Client::updateGame() {
    assert(this->game_ != nullptr);
    this->game_->update();
}

void Client::requestSceneSwap() {
    assert(!this->nextScene_.empty());
    if (this->state_ == State::Offline) {
        this->game_->loadScene(this->nextScene_);
    } else {
        this->netClient_.session().postMessage(net::MessageLoadSceneRequest{
            .generation = this->generation_,
            .sceneName = this->nextScene_,
        });
    }
}

void Client::setNextScene(std::string_view name) {
    this->nextScene_ = name;
}

void Client::render() {
    Renderer::renderClear();
    this->game_->render();
}

void Client::connect(std::string_view host, std::string_view port) {
    // Connect to host
    this->state_ = State::Connecting;
    this->netClient_.connect(host, port);
    this->netClient_.session().postMessage(net::MessageHello{});
}

void Client::disconnect() {
    // Reset client state
    this->state_ = State::Offline;
    this->clientID_ = 0;
    this->serverTickRate_ = 0;
    this->generation_ = 0;
    this->roomState_.clear();

    // Go to "disconnected" scene
    auto disconnectedScene =
        this->clientConfig_.disconnected_scene.value_or(this->clientConfig_.initial_scene);
    this->game_->loadScene(disconnectedScene);
}

void Client::processNetwork() {
    if (this->netClient_.session().stopped()) {
        // Session is apparently stopped. Mark current state as disconnected.
        this->disconnect();
        return;
    }

    // Consume all messages that have been received since the last call to
    // processNetwork().
    this->netClient_.session().consumeAllMessages([&](std::unique_ptr<net::SMessage> msg) {
        this->processMessage(std::move(msg));
    });
}

//-----------------------------------------------------------------------------
// Server message processing

void Client::processMessage(std::unique_ptr<net::SMessage> msg) {
    std::visit(
        [this](auto &m) {
            this->processMessage(m);
        },
        *msg);
}

void Client::processMessage(const net::MessageError &m) {}

void Client::processMessage(const net::MessageWelcome &m) {
    assert(this->state_ == State::Connecting); // TODO: something other than assert
    std::cout << "Joined host as client " << m.clientID << std::endl;
    // Store assigned client ID
    this->clientID_ = m.clientID;

    // Store server tick rate and set last replication time to current time.
    // This will sync (outside of network latency) replications to the server
    // tick rate because messages are always sent on ticks.
    this->serverTickRate_ = m.serverTickRate;
    this->game_ = std::make_unique<game::Game>(this->gameConfig_);
    this->game_->setTickDuration(
        std::chrono::microseconds(static_cast<int>(1000000.0F / m.serverTickRate)));
    this->lastReplication_ = std::chrono::steady_clock::now();

    // Update state
    this->state_ = State::Connected;
}

void Client::processMessage(const net::MessageLoadScene &m) {
    assert(this->state_ == State::Connected);
    if (this->game_ == nullptr) {
        std::cerr << "warning: received MessageLoadScene without game" << std::endl;
        return;
    }

    // Clear any pending replications
    this->replicatorService_.clear();

    // Immediately load the new scene in the game.
    this->generation_ = m.generation;
    this->game_->loadScene(m.sceneName);
    auto &scene = this->game_->currentScene();

    // Instantiate any runtime actors.
    for (const auto &runtimeActor : m.runtimeActors) {
        // Actually create the actor based on the template
        auto* a = scene.instantiateRuntimeActor(runtimeActor.actorTemplate, runtimeActor.owner);
        // Register the ID of the runtime actor on the server
        scene.registerActorRemoteID(a, runtimeActor.id);
    }

    // Synchronize scene state of all server_replicated components.
    for (const auto &req : m.sceneState) {
        // No interp on initial scene state
        net::ReplicatorService::dispatchReplication(*this->game_, req, false);
    }
}

void Client::processMessage(const net::MessageTickReplication &m) {
    assert(this->state_ == State::Connected);
    if (this->game_ == nullptr) {
        std::cerr << "warning: received MessageTickReplication without game" << std::endl;
        return;
    }
    // The server is informing us that the game state has been updated.
    // 1. Instantiate any new actors.
    // 2. Process any replication requests.
    // 3. Destroy any actors that need to be destroyed.
    auto &scene = this->game_->currentScene();

    for (const auto &instantiation : m.instantiations) {
        // Actually create the actor based on the template
        auto* a = scene.instantiateRuntimeActor(instantiation.actorTemplate, instantiation.owner);
        // Register the ID of the runtime actor on the server
        scene.registerActorRemoteID(a, instantiation.id);
        // Deserialize the component states of the instantiated actor
        net::ReplicatorService::dispatchReplication(a, instantiation.componentState);
    }

    for (const auto &req : m.replications) {
        // Perform interp on tick replications
        net::ReplicatorService::dispatchReplication(*this->game_, req, true);
    }

    for (auto id : m.destructions) {
        // Find the actor to destroy
        auto* a = scene.findActorByRemoteID(id);
        if (a == nullptr) {
            continue;
        }
        // Tell the actor the server has requested it to be destroyed. If the actor
        // is configured to defer the destruction, it will continue to live until
        // a lifecycle function eventually calls Actor.Destroy on a.
        a->serverRequestedDestroy();
    }
}

void Client::processMessage(const net::MessageTickReplicationAck &m) {
    assert(this->state_ == State::Connected);
    if (this->game_ == nullptr) {
        std::cerr << "warning: received MessageTickReplicationAck without game" << std::endl;
        return;
    }
    // The server is informing us that our MessageTickReplication that we
    // sent it has been acknowledged and that any instantiated actors have
    // been assigned a remote id. Create the local id <-> remote id mapping.
    auto &scene = this->game_->currentScene();

    for (const auto &remoteMapping : m.remoteIDMappings) {
        // Locate local actor that we instantiated
        auto* a = scene.findActorByID(remoteMapping.clientID);
        if (a == nullptr) {
            continue;
        }
        // Register the remote id
        scene.registerActorRemoteID(a, remoteMapping.serverID);
    }
}

void Client::processMessage(const net::MessageTickReplicationReject &m) {
    assert(this->state_ == State::Connected);
    if (this->game_ == nullptr) {
        std::cerr << "warning: received MessageTickReplicationReject without game" << std::endl;
        return;
    }
    // The server is informing us that our MessageTickReplication that we
    // sent it has been rejected. A new scene has been loaded on the server.
    auto &scene = this->game_->currentScene();

    for (const auto &rejectedID : m.rejectedInstantiations) {
        // Locate the actor that failed instantiation
        auto* a = scene.findActorByID(rejectedID);
        if (a == nullptr) {
            continue;
        }
        // Destroy that actor
        a->destroyLocally();
    }
}

void Client::processMessage(const net::MessageRoomState &m) {
    assert(this->state_ == State::Connected);
    assert(this->clientID_ != 0);
    if (this->game_ == nullptr) {
        std::cerr << "warning: received MessageRoomState without game" << std::endl;
        return;
    }
    using namespace std::string_view_literals;
    // The server is informing us that a client has joined or left the game.
    // Note that this is also sent when this client instance joins the game.
    std::vector<client_id_t> newlyJoined;
    std::vector<client_id_t> newlyLeft;
    auto newRoomState = std::set<client_id_t>{m.joinedClients.begin(), m.joinedClients.end()};

    for (const auto &client : newRoomState) {
        if (!this->roomState_.contains(client) && client != this->clientID_) {
            // Current client room state doesn't know about this client, it must
            // have just joined the game
            newlyJoined.push_back(client);
        }
    }

    for (const auto &client : this->roomState_) {
        if (!newRoomState.contains(client)) {
            // Received room state no longer contains this client, it must have
            // just left the game
            newlyLeft.push_back(client);
        }
    }

    // Update internal room state
    this->roomState_ = std::move(newRoomState);

    if (newlyJoined.empty() && newlyLeft.empty()) {
        return;
    }

    this->doAfterUpdate([this, j = std::move(newlyJoined), l = std::move(newlyLeft)] {
        // Dispatch joins/leaves to the game's EventSub instance
        for (const auto &joined : j) {
            this->game_->eventSub().publish(events::MultiplayerOnClientJoin, joined);
        }
        for (const auto &left : l) {
            this->game_->eventSub().publish(events::MultiplayerOnClientLeave, left);
        }
    });
}

void Client::processMessage(net::MessageRemoteEvents &m) {
    assert(this->state_ == State::Connected);
    if (this->game_ == nullptr) {
        std::cerr << "warning: received MessageRemoteEvents without game" << std::endl;
        return;
    }
    this->doAfterUpdate([this, publishes = std::move(m.publishes)] {
        // Dispatch all events to the game's EventSub instance
        for (const auto &p : publishes) {
            this->game_->eventSub().publish(p.event, p.value);
        }
    });
}

//-----------------------------------------------------------------------------

void Client::executeReplications() {
    auto now = std::chrono::steady_clock::now();
    if (!this->replicationRequired(now)) {
        return;
    }
    this->lastReplication_ = now;

    this->executeTickReplication();
    this->executeRemoteEvents();
}

bool Client::replicationRequired(const std::chrono::steady_clock::time_point &now) const {
    auto dt = std::chrono::duration_cast<std::chrono::microseconds>(now - this->lastReplication_);
    return dt >= this->game_->tickDuration();
}

void Client::executeTickReplication() {
    if (!this->replicatorService_.hasPendingReplications()) {
        // No replication data to send
        return;
    }

    auto instantiations = this->replicatorService_.serializeInstantiations();
    auto replications = this->replicatorService_.serializeComponents();
    auto destructions = this->replicatorService_.serializeDestructions();
    if (instantiations.empty() && replications.empty() && destructions.empty()) {
        // I think that this is possible if serializeComponents filters out
        // runtime-instantiated actors that haven't been TickReplicationAck'd yet.
        return;
    }

    this->netClient_.session().postMessage(net::MessageTickReplication{
        .generation = this->generation_,
        .instantiations = std::move(instantiations),
        .replications = std::move(replications),
        .destructions = std::move(destructions),
    });
}

void Client::executeRemoteEvents() {
    if (!this->replicatorService_.hasPendingEventPublishes()) {
        // No remote events to send
        return;
    }

    auto publishes = this->replicatorService_.serializeEventPublishes();
    assert(!publishes.empty());

    this->netClient_.session().postMessage(net::MessageRemoteEvents{
        .generation = this->generation_,
        .publishes = std::move(publishes),
    });
}

void Client::registerInternalEvents() {}

void Client::doAfterUpdate(std::function<void()> f) {
    this->afterUpdate_.emplace_back(std::move(f));
}

void Client::executeAfterUpdates() {
    for (const auto &after : this->afterUpdate_) {
        std::invoke(after);
    }
    this->afterUpdate_.clear();
}

std::unique_ptr<Client> EngineClient = nullptr;

void InitClient(resources::ClientConfig clientConfig, resources::GameConfig gameConfig,
                boost::asio::io_context &ioContext) {
    EngineClient =
        std::make_unique<Client>(std::move(clientConfig), std::move(gameConfig), ioContext);
}

void DeinitClient() {
    EngineClient->destroy();
    EngineClient.reset();
}

Client &CurrentClient() {
    assert(EngineClient != nullptr);
    return *EngineClient;
}

} // namespace client

game::Game &CurrentGame() {
    return *client::CurrentClient().game_;
}

game::Scene &CurrentScene() {
    return CurrentGame().currentScene();
}

bool GameOffline() {
    return client::CurrentClient().gameOffline();
}

} // namespace sge
