#pragma once

#include <boost/asio.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/channel.hpp>

namespace sge::util {

/**
 * @brief Mutex implemented with ASIO channels.
 */
class AsyncLock {
public:
    AsyncLock(const boost::asio::any_io_executor &executor);

    /**
     * @brief Acquire the lock, blocking.
     * 
     * @return boost::asio::awaitable<void> 
     */
    boost::asio::awaitable<void> lock();

    /**
     * @brief Release the lock.
     */
    void unlock();

private:
    boost::asio::experimental::channel<void()> chan_;
};

/**
 * @brief RAII-style wrapper for AsyncLock. Acquire with LockGuard function.
 */
class AsyncGuard {
public:
    ~AsyncGuard();

    AsyncGuard(const AsyncGuard &) = delete;
    AsyncGuard &operator=(const AsyncGuard &) = delete;

    AsyncGuard(AsyncGuard &&) noexcept;
    AsyncGuard &operator=(AsyncGuard &&) = delete;

private:
    friend boost::asio::awaitable<AsyncGuard> LockGuard(AsyncLock &lock);
    AsyncGuard(AsyncLock &lock_);

    AsyncLock &lock_;
    bool held_;
};

/**
 * @brief Acquire an AsyncLock, owned by an AsyncGuard.
 * 
 * @param lock AsyncLock to acquire
 * @return boost::asio::awaitable<AsyncGuard> Guard
 */
boost::asio::awaitable<AsyncGuard> LockGuard(AsyncLock &lock);

} // namespace sge::util
