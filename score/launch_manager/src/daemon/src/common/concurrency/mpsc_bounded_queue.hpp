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

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

#include "concurrency_error_domain.hpp"

#include <score/assert.hpp>
#include <score/expected.hpp>

namespace score::lcm::internal
{

/// @brief Fixed-size FIFO queue
/// @details Uses std::optional to eliminate default construction of type T elements at construction
/// @tparam T The type of elements stored in the queue. Supports move-only and copy-only types.
template <typename T>
class FixedSizeQueue
{
  public:
    /// @brief Constructs a FixedSizeQueue with a fixed runtime capacity.
    /// @details If the provided size is 0, the capacity automatically falls back to 1.
    /// @param size The desired maximum number of elements.
    FixedSizeQueue(size_t size) : capacity_((size == 0U) ? 1U : size) {
        slots_.resize(capacity_); 
    }

    /// @brief Inserts a new element directly at the tail of the queue.
    /// @tparam Args Variadic template arguments forwarded to the constructor of T.
    /// @param args The arguments used to construct the object of type T.
    /// @return true if the element was successfully inserted; false if the FIFO is full (overflow protection).
    template <typename... Args>
    bool push(Args&&... args) {
        if(full()) {
            return false;
        }

        slots_[tail_].emplace(std::forward<Args>(args)...);
        tail_ = (tail_ + 1U) % capacity_;
        count_++;
        
        return true;
    }

    
    /// @brief Attempts to extract and remove the oldest element from the queue.
    /// @details The popped element is immediately destroyed in the internal buffer to free resources.
    /// @return A std::optional containing the element, or std::nullopt if the queue was empty (underflow protection).
    std::optional<T> tryPop() {
        if (empty())
        {
            return std::nullopt;
        }

        std::optional<T> item = std::move(slots_[head_]);
        slots_[head_] = std::nullopt; 
        head_ = (head_ + 1U) % capacity_;
        count_--;

        return item;
    }

    /// @brief Checks if the queue contains no elements.
    /// @return true if empty, otherwise false.
    bool empty() const { return (count_ == 0U); }

    /// @brief Checks if the queue has reached its maximum capacity.
    /// @return true if full, otherwise false.
    bool full() const { return (count_ >= capacity_); }

    /// @brief Retrieves the current number of active elements in the queue.
    /// @return The count of stored elements.
    size_t size() const { return count_;}; 

    /// @brief Retrieves the capacity of the queue.
    /// @return The capacity of the queue
    size_t capacity() const { return capacity_;}; 


  private:
    /// @brief Internal buffer holding the optional elements.
    std::vector<std::optional<T>> slots_;
    /// @brief Index of the oldest element in the buffer (read index).
    size_t head_ = 0U;
    /// @brief Index where the next element will be inserted (write index).
    size_t tail_ = 0U;
    /// @brief Current number of active elements in the queue.
    size_t count_ = 0U;
    /// @brief Maximum capacity of the queue.
    size_t capacity_;
};

/// @brief Fixed-capacity queue for the multi-producer / single-consumer case,
///        built on a plain std::mutex + std::condition_variable rather than a lock-free scheme.
/// @details Unlike MPMCConcurrentQueue, T does not need to be default-constructible: slots are
///          std::optional<T>, emplaced/reset under the lock rather than pre-constructed in place.
/// @warning Only one thread shall call wait()/tryPop(). Multiple threads can call push() concurrently.
/// @warning push() never blocks: if the queue is full, it returns ConcurrencyErrc::kOverflow.
template <typename T>
class MpscBoundedQueue
{

  public:
    MpscBoundedQueue() = delete;

    /// @brief Constructs a MpscBoundedQueue with a fixed runtime capacity.
    /// @details If the provided size is 0, the capacity automatically falls back to 1.
    /// @param size The desired maximum number of elements.
    MpscBoundedQueue(size_t size) : queue_((size == 0U) ? 1U : size) {
    }

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
            return queue_.size() > 0U || stopped_;
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

        return queue_.tryPop();
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

        if (!queue_.push(std::move(item)))
        {
            return score::cpp::make_unexpected(ConcurrencyErrc::kOverflow);
        }

        lock.unlock();
        not_empty_cv_.notify_one();
        return {};
    }

    mutable std::mutex mutex_{};
    /// @brief Signaled by push()/stop(); waited on by wait().
    std::condition_variable not_empty_cv_{};
    /// @brief Queue holding the elements.
    FixedSizeQueue<T> queue_;
    /// @brief S
    bool stopped_{false};
    /// @brief Consumer thread id.
    std::thread::id consumer_thread_id_{};
};

}  // namespace score::lcm::internal

#endif  // MPSC_BOUNDED_QUEUE_HPP_INCLUDED
