#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/detail/error_code.hpp>

#include "net/Messages.hpp"
#include "net/Protocol.hpp"
#include "util/AsyncLock.hpp"

#include <cstddef>
#include <exception>
#include <memory>
#include <msgpack.hpp>
#include <span>
#include <utility>
#include <vector>

#if defined(NET_DEBUG)
#    include <iostream>
#endif

namespace sge::net {

template <class ReadMessage, class WriteMessage>
class MessageSocket {
public:
    MessageSocket(const boost::asio::any_io_executor &ioExecutor)
        : socket_{std::make_unique<boost::asio::ip::tcp::socket>(ioExecutor)}
        , lock_{ioExecutor} {}

    MessageSocket(boost::asio::ip::tcp::socket socket)
        : socket_{std::make_unique<boost::asio::ip::tcp::socket>(std::move(socket))}
        , lock_{this->socket_->get_executor()} {}

    ~MessageSocket() = default;

    /**
     * @brief Get a reference to the underlying socket.
     */
    boost::asio::ip::tcp::socket &socket() {
        return *this->socket_;
    }

    /**
     * @brief Connect the socket to the remote endpoint.
     * 
     * @param endpoint Endpoint to connect to.
     */
    boost::asio::awaitable<void> connect(const boost::asio::ip::tcp::endpoint &endpoint) {
        co_await this->socket_->async_connect(endpoint, boost::asio::use_awaitable);
    }

    /**
     * @brief Shutdown and close the socket.
     */
    void stop() {
        this->stopped_ = true;
        if (this->socket_->is_open()) {
            try {
                this->socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                this->socket_->close();
            } catch (std::exception &) {}
        }
    }

    bool stopped() const {
        return this->stopped_;
    }

    /**
     * @brief Read a message from the socket.
     */
    boost::asio::awaitable<std::unique_ptr<ReadMessage>> readMessage() {
        auto guard = util::LockGuard(this->lock_);
        // 1. Read message from socket
        std::size_t size = co_await ReadMessageAsync(*this->socket_, this->readBuffer_);
        // 2. Parse message
        auto data = std::span<const char>{this->readBuffer_.data(), size};
        auto msg = ParseMessage<ReadMessage>(data);
#if defined(NET_DEBUG)
        if (msg) {
            std::cout << "[ sock " << this->socket_->remote_endpoint() << " ] recv "
                      << StringOfMessageType(MessageTypeOfMessage(*msg)) << std::endl;
        }
#endif
        co_return msg;
    }

    /**
     * @brief Send message over the socket.
     * 
     * @param msg Message to send.
     */
    template <TypedMessage Msg>
    boost::asio::awaitable<void> writeMessage(const Msg &msg) {
        auto guard = util::LockGuard(this->lock_);
        // 1. Serialize message to buffer
        this->writeBuffer_.clear();
        SerializeMessage(this->writeBuffer_, msg);
        // 2. Write data to socket
        auto data = std::span<const char>{this->writeBuffer_.data(), this->writeBuffer_.size()};
#if defined(NET_DEBUG)
        std::cout << "[ sock " << this->socket_->remote_endpoint() << " ] send "
                  << StringOfMessageType(Msg::Mty) << std::endl;
#endif
        co_await WriteMessageAsync(*this->socket_, data);
    }

    /**
     * @brief Send a message over the socket.
     * 
     * @param msg Message to send.
     */
    boost::asio::awaitable<void> writeMessage(const WriteMessage &msg) {
        auto guard = util::LockGuard(this->lock_);
        // 1. Serialize message to buffer
        this->writeBuffer_.clear();
        SerializeMessage(this->writeBuffer_, msg);
        // 2. Write data to socket
        auto data = std::span<const char>{this->writeBuffer_.data(), this->writeBuffer_.size()};
#if defined(NET_DEBUG)
        std::cout << "[ sock " << this->socket_->remote_endpoint() << " ] send "
                  << StringOfMessageType(MessageTypeOfMessage(msg)) << std::endl;
#endif
        co_await WriteMessageAsync(*this->socket_, data);
    }

private:
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_;

    util::AsyncLock lock_;
    std::vector<char> readBuffer_;
    msgpack::sbuffer writeBuffer_;

    bool stopped_{false};
};

} // namespace sge::net
