#include "net/Host.hpp"

#include <boost/asio.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/completion_condition.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/lock_types.hpp>

#include "net/MessageSocket.hpp"
#include "net/Messages.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <utility>

namespace sge::net {

using boost::asio::detached;
using boost::asio::use_awaitable;
using boost::asio::ip::tcp;

TcpClientConnection::TcpClientConnection(client_id_t clientID, tcp::socket socket,
                                         std::weak_ptr<Host> host)
    : clientID_(clientID)
    , socket_(std::move(socket))
    , host_(std::move(host))
    , outgoingQueue_{this->socket_.socket().get_executor()} {}

TcpClientConnection::pointer TcpClientConnection::create(client_id_t clientID, tcp::socket socket,
                                                         std::weak_ptr<Host> host) {
    return pointer(new TcpClientConnection(clientID, std::move(socket), std::move(host)));
}

void TcpClientConnection::start() {
    // Spawn reader coroutine
    boost::asio::co_spawn(
        this->socket_.socket().get_executor(),
        [self = shared_from_this()] {
            return self->reader();
        },
        detached);

    // Spawn writer coroutine
    boost::asio::co_spawn(
        this->socket_.socket().get_executor(),
        [self = shared_from_this()] {
            return self->writer();
        },
        detached);
}

MessageSocket<CMessage, SMessage> &TcpClientConnection::socket() {
    return this->socket_;
}

void TcpClientConnection::postMessage(std::unique_ptr<SMessage> msg) {
    bool pushed = this->outgoingQueue_.push(msg);
    if (!pushed) {
        std::cerr << "warning: failed to push outgoing message onto queue" << std::endl;
    }
}

boost::asio::awaitable<void> TcpClientConnection::reader() {
    auto host = this->host_.lock();
    if (!host) {
        std::cerr << "client reader: failed to get pointer to host" << std::endl;
        co_return;
    }

    try {
        while (true) {
            // Read message from remote client
            auto msg = co_await this->socket_.readMessage();
            if (!msg) {
                continue;
            }
            // Dispatch message to host for processing
            host->processMessage(this->clientID_, std::move(msg));
        }
    } catch (const boost::system::system_error &e) {
        this->exceptionEncountered(e);
    }
}

boost::asio::awaitable<void> TcpClientConnection::writer() {
    try {
        while (true) {
            auto outboundMsg = co_await this->outgoingQueue_.async_pop();
            assert(outboundMsg != nullptr);
            co_await this->socket_.writeMessage(*outboundMsg);
        }
    } catch (const boost::system::system_error &e) {
        this->exceptionEncountered(e);
    }
}

void TcpClientConnection::exceptionEncountered(const boost::system::system_error &e) {
    if (e.code() != boost::asio::error::eof) {
        std::cerr << "tcp client connection: system error: " << e.what() << std::endl;
    }
    if (!this->stopped_) {
        this->stop();
        this->removeConnection();
    }
}

void TcpClientConnection::stop() {
    if (this->stopped_) {
        return;
    }
    // Shutdown and stop socket
    std::cout << "Stopping client connection " << this->clientID_ << std::endl;
    this->stopped_ = true;
    this->socket_.stop();
}

void TcpClientConnection::removeConnection() {
    if (auto host = this->host_.lock()) {
        host->disconnectClient(this->clientID_);
    }
}

Host::Host(boost::asio::io_context &ioContext, int port)
    : acceptor_{ioContext, tcp::endpoint(tcp::v4(), port)}
    , messageQueue_{ioContext.get_executor()}
    , clientEventQueue_{ioContext.get_executor()} {
    boost::asio::socket_base::reuse_address option(true);
    this->acceptor_.set_option(option);
}

Host::pointer Host::create(boost::asio::io_context &ioContext, int port) {
    return pointer(new Host(ioContext, port));
}

void Host::start() {
    // Begin accepting connections
    boost::asio::co_spawn(
        this->acceptor_.get_executor(),
        [self = shared_from_this()] {
            return self->listen();
        },
        detached);
}

void Host::disconnectClient(client_id_t id) {
    {
        boost::lock_guard guard(this->mu_);
        auto it = this->connections_.find(id);
        if (it == this->connections_.end()) {
            return;
        }
        it->second->stop();
        this->connections_.erase(it);
    }

    this->clientEventQueue_.push(ClientEvent{
        .clientID = id,
        .event = ClientEventType::Disconnected,
    });
}

boost::asio::awaitable<void> Host::listen() {
    while (true) {
        auto sock = co_await this->acceptor_.async_accept(use_awaitable);
        auto clientID = this->nextClientID_++;
        auto conn = TcpClientConnection::create(clientID, std::move(sock), weak_from_this());
        {
            boost::lock_guard guard(this->mu_);
            this->connections_.emplace(clientID, conn);
        }

        this->clientEventQueue_.push(ClientEvent{
            .clientID = clientID,
            .event = ClientEventType::Connected,
        });
        conn->start();
    }
}

void Host::processMessage(client_id_t clientID, std::unique_ptr<CMessage> msg) {
    bool pushed = this->messageQueue_.push(ClientMessage{
        .clientID = clientID,
        .msg = std::move(msg),
    });
    if (!pushed) {
        std::cerr << "warning: failed to push received client message to queue" << std::endl;
    }
}

void Host::postMessage(client_id_t clientID, const SMessage &msg) {
    boost::shared_lock guard(this->mu_);
    auto it = this->connections_.find(clientID);
    if (it == this->connections_.end()) {
        return;
    }
    it->second->postMessage(std::make_unique<SMessage>(msg));
}

void Host::postMessage(client_id_t clientID, SMessage &&msg) {
    boost::shared_lock guard(this->mu_);
    auto it = this->connections_.find(clientID);
    if (it == this->connections_.end()) {
        return;
    }
    it->second->postMessage(std::make_unique<SMessage>(std::move(msg)));
}

void Host::broadcastMessage(const SMessage &msg) {
    boost::shared_lock guard(this->mu_);
    for (const auto &connEntry : this->connections_) {
        connEntry.second->postMessage(std::make_unique<SMessage>(msg));
    }
}

} // namespace sge::net
