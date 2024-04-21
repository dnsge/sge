#include "net/Client.hpp"

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/yield.hpp>
#include <boost/system/detail/error_code.hpp>

#include "net/Messages.hpp"

#include <cassert>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace sge::net {

using boost::asio::detached;
using boost::asio::use_awaitable;

Session::Session(const boost::asio::any_io_executor &ioExecutor)
    : socket_{ioExecutor}
    , messageQueue_{ioExecutor}
    , outgoingQueue_{ioExecutor} {}

Session::~Session() {
    this->stop();
}

Session::pointer Session::create(const boost::asio::any_io_executor &ioExecutor) {
    return pointer(new Session(ioExecutor));
}

boost::asio::awaitable<void> Session::connect(const tcp::endpoint &endpoint) {
    try {
        // Open socket
        co_await this->socket_.connect(endpoint);
    } catch (std::exception &e) {
        std::cerr << "connect: " << e.what() << std::endl;
        this->stop();
        co_return;
    }

    this->spawnWorkers();
}

tcp::socket &Session::socket() {
    return this->socket_.socket();
}

void Session::stop() {
    this->socket_.stop();
}

bool Session::stopped() const {
    return this->socket_.stopped();
}

void Session::postMessage(const CMessage &msg) {
    auto owned = std::make_unique<CMessage>(msg);
    bool pushed = this->postMessage(owned);
    if (!pushed) {
        std::cerr << "warning: failed to push outgoing message onto queue" << std::endl;
    }
}

void Session::postMessage(CMessage &&msg) {
    auto owned = std::make_unique<CMessage>(std::move(msg));
    bool pushed = this->postMessage(owned);
    if (!pushed) {
        std::cerr << "warning: failed to push outgoing message onto queue" << std::endl;
    }
}

bool Session::postMessage(std::unique_ptr<CMessage> &msg) {
    return this->outgoingQueue_.push(msg);
}

void Session::spawnWorkers() {
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

boost::asio::awaitable<void> Session::reader() {
    try {
        while (true) {
            // Read message from host
            auto msg = co_await this->socket_.readMessage();
            if (!msg) {
                continue;
            }
            // Add message to queue to be processed
            bool pushed = this->messageQueue_.push(msg);
            if (!pushed) {
                std::cerr << "warning: failed to push received message to queue" << std::endl;
            }
        }
    } catch (std::exception &) {
        this->stop();
    }
}

boost::asio::awaitable<void> Session::writer() {
    try {
        while (true) {
            auto outboundMsg = co_await this->outgoingQueue_.async_pop();
            assert(outboundMsg != nullptr);
            co_await this->socket_.writeMessage(*outboundMsg);
        }
    } catch (std::exception &) {
        this->stop();
    }
}

Client::Client(boost::asio::io_context &ioContext)
    : ioContext_(&ioContext)
    , session_{nullptr} {}

void Client::connect(std::string_view host, std::string_view port) {
    tcp::resolver resolver(this->ioContext_->get_executor());
    auto i = resolver.resolve(host, port, tcp::resolver::numeric_service);

    std::optional<tcp::endpoint> best{std::nullopt};

    for (; !i.empty(); ++i) {
        if (!best.has_value()) {
            best = *i;
        } else if (best->protocol() == tcp::v6() && i->endpoint().protocol() == tcp::v4()) {
            best = *i;
            break;
        }
    }

    if (best.has_value()) {
        this->connect(*best);
    } else {
        throw std::runtime_error("failed to resolve host/port endpoint");
    }
}

void Client::connect(const tcp::endpoint &endpoint) {
    // Create new session
    if (this->session_ != nullptr) {
        this->session_->stop();
    }
    this->session_ = Session::create(this->ioContext_->get_executor());

    // Spawn coroutine to connect to remote host
    boost::asio::co_spawn(
        this->session_->socket().get_executor(),
        [=, conn = this->session_] {
            return conn->connect(endpoint);
        },
        detached);
}

Session &Client::session() const {
    assert(this->session_ != nullptr);
    return *this->session_;
}

} // namespace sge::net
