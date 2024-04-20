#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/impl/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "net/MessageSocket.hpp"
#include "net/Messages.hpp"
#include "util/AsyncSpscQueue.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <msgpack.hpp>
#include <unordered_map>
#include <utility>

namespace sge::net {

using boost::asio::ip::tcp;

class Host;

struct ClientMessage {
    client_id_t clientID;
    std::unique_ptr<CMessage> msg;
};

enum class ClientEventType {
    Connected,
    Disconnected
};

struct ClientEvent {
    client_id_t clientID;
    ClientEventType event;
};

class TcpClientConnection : public std::enable_shared_from_this<TcpClientConnection> {
public:
    using pointer = std::shared_ptr<TcpClientConnection>;
    static pointer create(client_id_t clientID, tcp::socket socket, std::weak_ptr<Host> host);

    /**
     * @brief Spawn the client connection coroutines
     */
    void start();

    /**
     * @brief Shutdown and close the TCP socket.
     */
    void stop();

    MessageSocket<CMessage, SMessage> &socket();

    void postMessage(std::unique_ptr<SMessage> msg);

private:
    TcpClientConnection(client_id_t clientID, tcp::socket socket, std::weak_ptr<Host> host);

    /**
     * @brief Process reads from the TCP socket.
     */
    boost::asio::awaitable<void> reader();

    /**
     * @brief Write messages in the outgoing queue.
     */
    boost::asio::awaitable<void> writer();

    void exceptionEncountered(const boost::system::system_error &e);

    /**
     * @brief Remove this connection from the Host connections map
     */
    void removeConnection();

    client_id_t clientID_;
    MessageSocket<CMessage, SMessage> socket_;
    std::weak_ptr<Host> host_;

    util::AsyncSpscQueue<SMessage> outgoingQueue_;

    std::atomic<bool> stopped_{false};
};

class Host : public std::enable_shared_from_this<Host> {
public:
    using pointer = std::shared_ptr<Host>;
    static pointer create(boost::asio::io_context &ioContext, int port);

    void start();
    void disconnectClient(client_id_t id);

    void processMessage(client_id_t clientID, std::unique_ptr<CMessage> msg);

    void postMessage(client_id_t clientID, const SMessage &msg);
    void postMessage(client_id_t clientID, SMessage &&msg);
    void broadcastMessage(const SMessage &msg);

    /**
     * @brief Conditionally broadcast a message to clients.
     * 
     * @param msg Message to broadcast.
     * @param p Predicate of client id to filter by.
     */
    template <typename Pred>
    void broadcastMessage(const SMessage &msg, Pred p) {
        boost::shared_lock guard(this->mu_);
        for (const auto &connEntry : this->connections_) {
            if (p(connEntry.first)) {
                connEntry.second->postMessage(std::make_unique<SMessage>(msg));
            }
        }
    }

    /**
     * @brief Attempt to consume one client message from the queue.
     * 
     * @param f Functor to process the consumed client message.
     * @return true If a message was consumed.
     * @return false If a message was not consumed.
     */
    template <typename F>
    bool consumeClientMessage(F &&f) {
        return this->messageQueue_.consume_one(std::forward<F>(f));
    }

    /**
     * @brief Attempt to consume all client messages in the queue.
     * 
     * @param f Functor to process the consumed client messages.
     * @return The number of messages consumed.
     */
    template <typename F>
    std::size_t consumeAllClientMessages(F &&f) {
        return this->messageQueue_.consume_all(std::forward<F>(f));
    }

    /**
     * @brief Attempt to consume all client events in the queue.
     * 
     * @param f Functor to process the consumed client events.
     * @return The number of events consumed.
     */
    template <typename F>
    std::size_t consumeAllClientEvents(F &&f) {
        return this->clientEventQueue_.consume_all(std::forward<F>(f));
    }

private:
    Host(boost::asio::io_context &ioContext, int port);

    boost::asio::awaitable<void> listen();

    tcp::acceptor acceptor_;

    boost::shared_mutex mu_;
    client_id_t nextClientID_{1};
    std::unordered_map<client_id_t, TcpClientConnection::pointer> connections_;

    util::AsyncSpscQueue<ClientMessage> messageQueue_;
    util::AsyncSpscQueue<ClientEvent, 10> clientEventQueue_;
};

} // namespace sge::net
