#pragma once

#include <SDL2/SDL.h>
#include <boost/asio/io_context.hpp>

#include "Common.hpp" // IWYU pragma: keep
#include "Types.hpp"
#include "game/Game.hpp"
#include "net/Client.hpp"
#include "net/Messages.hpp"
#include "net/Replicator.hpp"
#include "render/RenderQueue.hpp"
#include "resources/Configs.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace sge::client {

class Client {
public:
    Client(resources::ClientConfig clientConfig, resources::GameConfig gameConfig,
           boost::asio::io_context &ioContext);

    void run();
    void destroy();

    void connect(std::string_view host, std::string_view port);
    void disconnect();

    const resources::ClientConfig &config() const;
    net::Client &netClient();
    net::ReplicatorService &replicatorService();

    bool gameOffline() const;
    client_id_t clientID() const;
    std::vector<client_id_t> joinedClients() const;

    void setNextScene(std::string_view name);

private:
    friend game::Game & ::sge::CurrentGame();

    enum class State {
        Offline,
        Connecting,
        Connected,
    };

    void readInput();
    void update();
    void render();

    void updateGame();
    void requestSceneSwap();

    void processNetwork();
    void processMessage(std::unique_ptr<net::SMessage> msg);
    void processMessage(const net::MessageError &m);
    void processMessage(const net::MessageWelcome &m);
    void processMessage(const net::MessageLoadScene &m);
    void processMessage(const net::MessageTickReplication &m);
    void processMessage(const net::MessageTickReplicationAck &m);
    void processMessage(const net::MessageTickReplicationReject &m);
    void processMessage(const net::MessageRoomState &m);
    void processMessage(net::MessageRemoteEvents &m);

    void executeReplications();
    bool replicationRequired(const std::chrono::steady_clock::time_point &now) const;

    void executeTickReplication();
    void executeRemoteEvents();

    void registerInternalEvents();

    void doAfterUpdate(std::function<void()> f);
    void executeAfterUpdates();

    resources::ClientConfig clientConfig_;
    resources::GameConfig gameConfig_;

    net::Client netClient_;
    net::ReplicatorService replicatorService_;
    std::chrono::steady_clock::time_point lastReplication_{};

    std::unique_ptr<game::Game> game_{nullptr};
    render::RenderQueue renderQueue_{};

    bool running_{true};
    State state_{State::Offline};

    // Client connection state. Make sure to clean up when disconnecting.
    client_id_t clientID_{0};
    unsigned int serverTickRate_{0};
    unsigned int generation_{0};
    std::set<client_id_t> roomState_{};

    std::string nextScene_{};

    std::vector<std::function<void()>> afterUpdate_;
};

void InitClient(resources::ClientConfig clientConfig, resources::GameConfig gameConfig,
                boost::asio::io_context &ioContext);
void DeinitClient();

Client &CurrentClient();

} // namespace sge::client
