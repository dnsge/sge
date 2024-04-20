#include "net/Protocol.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/completion_condition.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace sge::net {

boost::asio::awaitable<std::size_t> ReadMessageAsync(socket &sock, std::vector<char> &buf) {
    std::size_t n;

    // 1. Read message size header
    uint32_t rawSize;
    n = co_await boost::asio::async_read(sock,
                                         boost::asio::buffer(&rawSize, sizeof(uint32_t)),
                                         boost::asio::transfer_exactly(sizeof(uint32_t)),
                                         boost::asio::use_awaitable);
    assert(n == sizeof(uint32_t));

    // 2. Convert network byte order to host byte order
    auto size = static_cast<std::size_t>(ntohl(rawSize));

    // 3. Read full length of message
    buf.reserve(size);
    n = co_await boost::asio::async_read(sock,
                                         boost::asio::mutable_buffer(buf.data(), size),
                                         boost::asio::transfer_exactly(size),
                                         boost::asio::use_awaitable);
    assert(n == size);

    co_return n;
}

boost::asio::awaitable<void> WriteMessageAsync(socket &sock, std::span<const char> msg) {
    // 1. Send size of data over network
    uint32_t rawSize = htonl(static_cast<uint32_t>(msg.size()));
    co_await boost::asio::async_write(
        sock, boost::asio::const_buffer(&rawSize, sizeof(uint32_t)), boost::asio::use_awaitable);

    // 2. Send actual message data over network
    co_await boost::asio::async_write(
        sock, boost::asio::const_buffer(msg.data(), msg.size()), boost::asio::use_awaitable);

    co_return;
}

} // namespace sge::net
