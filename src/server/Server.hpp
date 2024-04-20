#pragma once

#include <boost/asio/io_context.hpp>

#include "Common.hpp" // IWYU pragma: keep
#include "Types.hpp"
#include "game/Game.hpp"
#include "net/Host.hpp"
#include "net/Messages.hpp"
#include "net/Replicator.hpp"
#include "resources/Configs.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sge::server {

enum class ClientState {
    Initializing,
    Joined,
};

class Server {
public:
    Server(resources::ServerConfig serverConfig, resources::GameConfig gameConfig,
           boost::asio::io_context &ioContext);

    void run();

    net::ReplicatorService &replicatorService();

    unsigned int tickNum() const;
    std::vector<client_id_t> joinedClients() const;

    void setNextScene(std::string_view name);

private:
    friend game::Game & ::sge::CurrentGame();

    void tick();
    void initGame();
    void updateGame();
    void swapScene(const std::string &name);

    void clientJoined(client_id_t clientID);
    void clientLeft(client_id_t clientID);

    void processNetwork();
    void processMessage(std::unique_ptr<net::ClientMessage> msg);
    void processMessage(client_id_t clientID, const net::MessageError &m);
    void processMessage(client_id_t clientID, const net::MessageHello &m);
    void processMessage(client_id_t clientID, const net::MessageLoadSceneRequest &m);
    void processMessage(client_id_t clientID, net::MessageTickReplication &m);
    void processMessage(client_id_t clientID, net::MessageRemoteEvents &m);

    void executeReplications();
    void executeTickReplication();
    void executeRemoteEvents();
    void processReplicationRequest(const net::ComponentReplication &replication);

    void sendInvalidMessage(client_id_t clientID);
    void broadcastToJoined(const net::SMessage &msg);
    void broadcastToOthers(const net::SMessage &msg, client_id_t src);
    void broadcastRoomState();

    bool isJoined(client_id_t clientID) const;

    void doAfterUpdate(std::function<void()> f);
    void executeAfterUpdates();

    resources::ServerConfig serverConfig_;
    resources::GameConfig gameConfig_;

    net::Host::pointer host_;
    net::ReplicatorService replicatorService_;
    std::unordered_map<client_id_t, ClientState> clientStates_;

    std::unique_ptr<game::Game> game_{nullptr};

    bool running_{true};
    unsigned int tickNum_{0};
    unsigned int generation_{0};

    std::string nextScene_{};
    bool roomStateChanged_{false};

    std::vector<std::function<void()>> afterUpdate_;
};

void InitServer(resources::ServerConfig serverConfig, resources::GameConfig gameConfig,
                boost::asio::io_context &ioContext);
void DeinitServer();

Server &CurrentServer();

} // namespace sge::server
