#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/base.hpp>
#include <boost/lockfree/policies.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/system/detail/error_code.hpp>

#include <cassert>
#include <chrono>
#include <cstddef>
#include <memory>

namespace sge::util {

constexpr std::size_t DefaultSpscQueueCapacity = 1000;

// NOLINTBEGIN(readability-identifier-naming)

template <typename T, std::size_t Capacity = DefaultSpscQueueCapacity>
class AsyncSpscQueue {
public:
    using ptr = T*;
    using owning_ptr = std::unique_ptr<T>;

    AsyncSpscQueue(const boost::asio::any_io_executor &executor)
        : cv_(executor) {
        this->cv_.expires_at(std::chrono::steady_clock::time_point::max());
    }

    ~AsyncSpscQueue() {
        this->queue_.consume_all([](ptr ptr) {
            owning_ptr(ptr).reset();
        });
    }

    /**
     * @brief Attempt to push an item onto the queue.
     * 
     * @param item Item to push.
     * @return true If the push succeeds.
     * @return false If the push fails.
     */
    bool push(owning_ptr &item) {
        assert(item != nullptr);
        bool pushed = this->queue_.push(item.get());
        if (pushed) {
            // Successfully pushed, release ownership from owning_ptr
            item.release();
            // Notify async_pop that item is available
            boost::system::error_code ec;
            this->cv_.cancel_one(ec);
        }
        return pushed;
    }

    /**
     * @brief Attempt to push an item onto the queue.
     * 
     * @param item Item to push.
     * @return true If the push succeeds.
     * @return false If the push fails.
     */
    bool push(const T &item) {
        auto owned = std::make_unique<T>(item);
        return this->push(owned);
    }

    /**
     * @brief Attempt to push an item onto the queue.
     * 
     * @param item Item to push.
     * @return true If the push succeeds.
     * @return false If the push fails.
     */
    bool push(T &&item) {
        auto owned = std::make_unique<T>(std::move(item));
        return this->push(owned);
    }

    /**
     * @brief Attempt to pop an item from the queue.
     * 
     * @param out Output location of popped item.
     * @return true If the pop succeeds.
     * @return false If the pop fails.
     */
    [[nodiscard]] bool pop(owning_ptr &out) {
        ptr ptr = nullptr;
        bool popped = this->queue_.pop(ptr);
        if (popped) {
            // Give ownership of raw pointer to input owning_ptr
            out.reset(ptr);
            return true;
        }
        return false;
    }

    /**
     * @brief Asynchronously pop an item from the queue.
     * 
     * @return boost::asio::awaitable<owning_ptr> The popped item.
     */
    [[nodiscard]] boost::asio::awaitable<owning_ptr> async_pop() {
        ptr ptr = nullptr;
        bool popped = false;
        while (true) {
            popped = this->queue_.pop(ptr);
            if (popped) {
                // Take ownership of raw pointer, returning owning_ptr
                co_return owning_ptr(ptr);
            }
            // Wait for notification from cv timer
            boost::system::error_code ec;
            co_await this->cv_.async_wait(
                boost::asio::redirect_error(boost::asio::use_awaitable, ec));
        }
    }

    /**
     * @brief Attempt to consume one item from the queue.
     * 
     * @param f Functor to process the consumed item.
     * @return true If an item was consumed.
     * @return false If an item was not consumed.
     */
    template <typename F>
    bool consume_one(F f) {
        return this->queue_.consume_one([&](ptr ptr) {
            f(owning_ptr(ptr));
        });
    }

    /**
     * @brief Attempt to consume all items in the queue.
     * 
     * @param f Functor to process the consumed items.
     * @return The number of items consumed.
     */
    template <typename F>
    std::size_t consume_all(F f) {
        return this->queue_.consume_all([&](ptr ptr) {
            f(owning_ptr(ptr));
        });
    }

private:
    boost::lockfree::spsc_queue<ptr, boost::lockfree::capacity<Capacity>> queue_;
    boost::asio::steady_timer cv_;
};

// NOLINTEND(readability-identifier-naming)

} // namespace sge::util
