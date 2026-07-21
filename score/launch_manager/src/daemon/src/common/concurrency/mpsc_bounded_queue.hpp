/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#ifndef MPSC_BOUNDED_QUEUE_HPP_INCLUDED
#define MPSC_BOUNDED_QUEUE_HPP_INCLUDED

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

#include "concurrency_error_domain.hpp"

#include <score/assert.hpp>
#include <score/expected.hpp>

namespace score::lcm::internal
{

/// @brief Fixed-capacity queue for the multi-producer / single-consumer case,
///        built on a plain std::mutex + std::condition_variable rather than a lock-free scheme.
/// @details Unlike MPMCConcurrentQueue, T does not need to be default-constructible: slots are
///          std::optional<T>, emplaced/reset under the lock rather than pre-constructed in place.
/// @warning Only one thread shall call wait()/tryPop(). Multiple threads can call push() concurrently.
/// @warning push() never blocks: if the queue is full, it returns ConcurrencyErrc::kOverflow.
template <typename T, std::size_t Capacity>
class MpscBoundedQueue
{
    static_assert(Capacity > 0U, "Capacity must be at least 1");

  public:
    MpscBoundedQueue() = default;
    ~MpscBoundedQueue() {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(is_stopped(), "Call stop() and join/shut down all threads" 
            "that might still call wait()/tryPop()/push() only then let the queue be destroyed.");
    };

    MpscBoundedQueue(const MpscBoundedQueue&) = delete;
    MpscBoundedQueue& operator=(const MpscBoundedQueue&) = delete;
    MpscBoundedQueue(MpscBoundedQueue&&) = delete;
    MpscBoundedQueue& operator=(MpscBoundedQueue&&) = delete;

    /// @brief Enqueues an item.
    /// @return blank on success; ConcurrencyErrc::kOverflow if the queue is full, or
    ///         ConcurrencyErrc::kStopped if the queue has been stopped.
    [[nodiscard]] score::cpp::expected_blank<ConcurrencyErrc> push(T&& item)
    {
        return push_impl(std::move(item));
    }

    [[nodiscard]] score::cpp::expected_blank<ConcurrencyErrc> push(const T& item)
    {
        return push_impl(item);
    }

    /// @brief Waits up to `timeout` for at least one item to become available.
    /// @param timeout Maximum time to wait.
    /// @return blank if an item is available (caller should drain via tryPop());
    ///         ConcurrencyErrc::kTimeout if the timeout elapsed with none available, or
    ///         ConcurrencyErrc::kStopped if the queue has been stopped.
    [[nodiscard]] score::cpp::expected_blank<ConcurrencyErrc> wait(std::chrono::milliseconds timeout)
    {
        std::unique_lock lock(mutex_);
        SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(ensure_single_consumer(), "Only a single consumer thread is allowed.");

        const bool has_item = not_empty_cv_.wait_for(lock, timeout, [this] {
            return count_ > 0U || stopped_;
        });

        if (stopped_)
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kStopped);
        }
        if (!has_item)
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kTimeout);
        }
        return {};
    }

    /// @brief Non-blocking pop. Never waits, regardless of whether the queue has been stopped, so
    ///        that anything still queued at shutdown time can still be drained.
    /// @return The next item if any is available right now, or std::nullopt if empty.
    std::optional<T> tryPop()
    {
        std::unique_lock lock(mutex_);
        SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(ensure_single_consumer(), "Only a single consumer thread is allowed.");

        if (count_ == 0U)
        {
            return std::nullopt;
        }

        std::optional<T> item = std::move(slots_[head_]);
        slots_[head_].reset();
        head_ = (head_ + 1U) % Capacity;
        --count_;

        return item;
    }

    /// @brief Permanently marks the queue stopped and wakes every thread currently blocked in
    ///        wait(). After this call, push() and wait() always return
    ///        ConcurrencyErrc::kStopped. tryPop() is unaffected and continues to drain any
    ///        queued items.
    void stop()
    {
        {
            std::lock_guard lock(mutex_);
            stopped_ = true;
        }
        not_empty_cv_.notify_all();
    }

  private:

    /// @brief Checks whether the queue is stopped
    [[nodiscard]] bool is_stopped() {
        std::lock_guard lock(mutex_);
        return stopped_;
    }

    /// @brief Checks whether the consumer thread id is unchanged   
    [[nodiscard]] bool ensure_single_consumer()
    {
        if(consumer_thread_id_ == std::thread::id{}) {
            consumer_thread_id_ = std::this_thread::get_id();
        }
        return consumer_thread_id_== std::this_thread::get_id();
    }

    template <typename U>
    [[nodiscard]] score::cpp::expected_blank<ConcurrencyErrc> push_impl(U&& item)
    {
        std::unique_lock lock(mutex_);

        if (stopped_)
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kStopped);
        }

        if (count_ >= Capacity)
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kOverflow);
        }

        slots_[tail_].emplace(std::forward<U>(item));
        tail_ = (tail_ + 1U) % Capacity;
        ++count_;

        lock.unlock();
        not_empty_cv_.notify_one();
        return {};
    }

    mutable std::mutex mutex_;
    /// @brief Signaled by push()/stop(); waited on by wait().
    std::condition_variable not_empty_cv_;
    std::array<std::optional<T>, Capacity> slots_{};
    /// @brief Index of the oldest item, valid only when count_ > 0.
    std::size_t head_{0};
    /// @brief Index where the next push lands.
    std::size_t tail_{0};
    /// @brief Number of occupied slots, guarded by mutex_.
    std::size_t count_{0};
    /// @brief Guarded by mutex_.
    bool stopped_{false};
    /// @brief Consumer thread id
    std::thread::id consumer_thread_id_;
};

}  // namespace score::lcm::internal

#endif  // MPSC_BOUNDED_QUEUE_HPP_INCLUDED
