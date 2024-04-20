#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace sge::net {

using socket = boost::asio::ip::tcp::socket;

boost::asio::awaitable<std::size_t> ReadMessageAsync(socket &sock, std::vector<char> &buf);
boost::asio::awaitable<void> WriteMessageAsync(socket &sock, std::span<const char> msg);

} // namespace sge::net
