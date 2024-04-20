#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/detail/error_code.hpp>

#include "net/MessageSocket.hpp"
#include "net/Messages.hpp"
#include "util/AsyncSpscQueue.hpp"

#include <cstddef>
#include <memory>
#include <msgpack.hpp>
#include <string_view>

namespace sge::net {

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    using pointer = std::shared_ptr<Session>;
    static pointer create(const boost::asio::any_io_executor &ioExecutor);

    ~Session();

    /**
     * @brief Connect to the remote host and begin accepting messages
     * 
     * @param endpoint Host endpoint
     */
    boost::asio::awaitable<void> connect(const tcp::endpoint &endpoint);

    tcp::socket &socket();

    void stop();

    bool stopped() const;

    /**
     * @brief Post a message to send to the host.
     * 
     * @param msg Message to send.
     */
    void postMessage(const CMessage &msg);

    /**
     * @brief Post a message to send to the host.
     * 
     * @param msg Message to send.
     */
    void postMessage(CMessage &&msg);

    /**
     * @brief Post a message to send to the host.
     * 
     * @param msg Message to send.
     * @return true If the message was accepted for sending.
     * @return false If the message was not accepted for sending.
     */
    bool postMessage(std::unique_ptr<CMessage> &msg);

    /**
     * @brief Attempt to consume one message from the queue.
     * 
     * @param f Functor to process the consumed message.
     * @return true If a message was consumed.
     * @return false If a message was not consumed.
     */
    template <typename F>
    bool consumeMessage(F &&f) {
        return this->messageQueue_.consume_one(std::forward<F>(f));
    }

    /**
     * @brief Attempt to consume all messages in the queue.
     * 
     * @param f Functor to process the consumed messages.
     * @return The number of messages consumed.
     */
    template <typename F>
    std::size_t consumeAllMessages(F &&f) {
        return this->messageQueue_.consume_all(std::forward<F>(f));
    }

private:
    Session(const boost::asio::any_io_executor &ioExecutor);

    void spawnWorkers();
    boost::asio::awaitable<void> reader();
    boost::asio::awaitable<void> writer();

    MessageSocket<SMessage, CMessage> socket_;
    util::AsyncSpscQueue<SMessage> messageQueue_;
    util::AsyncSpscQueue<CMessage> outgoingQueue_;
};

class Client {
public:
    Client(boost::asio::io_context &ioContext);

    void connect(std::string_view host, std::string_view port);
    Session &session() const;

private:
    void connect(const tcp::endpoint &endpoint);

    boost::asio::io_context* ioContext_;
    Session::pointer session_;
};

} // namespace sge::net
