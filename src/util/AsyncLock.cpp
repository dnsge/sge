#include "util/AsyncLock.hpp"

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/experimental/channel.hpp>

namespace sge::util {

AsyncLock::AsyncLock(const boost::asio::any_io_executor &executor)
    : chan_{executor, 1} {}

boost::asio::awaitable<void> AsyncLock::lock() {
    co_await this->chan_.async_send(boost::asio::deferred);
}

void AsyncLock::unlock() {
    this->chan_.try_receive([](auto...) {});
}

AsyncGuard::AsyncGuard(AsyncLock &lock)
    : lock_(lock)
    , held_(true) {}

AsyncGuard::~AsyncGuard() {
    if (this->held_) {
        this->lock_.unlock();
    }
}
AsyncGuard::AsyncGuard(AsyncGuard &&other) noexcept
    : lock_(other.lock_)
    , held_(other.held_) {
    other.held_ = false;
}

boost::asio::awaitable<AsyncGuard> LockGuard(AsyncLock &lock) {
    co_await lock.lock();
    co_return AsyncGuard{lock};
}

} // namespace sge::util
