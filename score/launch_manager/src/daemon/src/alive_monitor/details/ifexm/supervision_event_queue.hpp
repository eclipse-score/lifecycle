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
#ifndef SUPERVISION_EVENT_QUEUE_HPP
#define SUPERVISION_EVENT_QUEUE_HPP

#include "score/mw/launch_manager/alive_monitor/details/ifexm/supervision_event.hpp"
#include "score/mw/launch_manager/common/concurrency/mpsc_bounded_queue.hpp"
#include <atomic>
#include <cstddef>

namespace score::mw::lifecycle
{

namespace BufferConstants
{

/// @brief Supervision event queue size
constexpr std::size_t BUFFER_QUEUE_SIZE = 4096UL;

}  // namespace BufferConstants

/// @brief Multi-producer, single-consumer queue of supervision events with a sticky overflow flag.
/// @details Drops the newest event when full. Only one thread may call tryDequeue().
class SupervisionEventQueue
{
  public:
    /// @brief Constructor
    /// @details Creates a queue with the given fixed capacity.
    /// @param [in] capacity    Maximum number of events held before new events are dropped.
    explicit SupervisionEventQueue(std::size_t capacity = BufferConstants::BUFFER_QUEUE_SIZE) : queue_(capacity)
    {
    }

    // The queue asserts it is stopped on destruction; no producer may be active at this point.
    ~SupervisionEventQueue()
    {
        queue_.stop();
    }

    SupervisionEventQueue(const SupervisionEventQueue&) = delete;
    SupervisionEventQueue& operator=(const SupervisionEventQueue&) = delete;
    SupervisionEventQueue(SupervisionEventQueue&&) = delete;
    SupervisionEventQueue& operator=(SupervisionEventQueue&&) = delete;

    /// @brief Try Enqueue
    /// @details Thread-safe for multiple producers. Drops the event and sets the sticky overflow flag when full.
    /// @param [in] event    Event to enqueue.
    /// @return true if enqueued, false otherwise.
    bool tryEnqueue(const SupervisionEvent& event)
    {
        return mapEnqueueResult(queue_.push(event), overflow_);
    }

    /// @brief Try Dequeue
    /// @details Must only be called from the single consumer thread; does not block.
    /// @param [out] event    Receives the oldest event; unchanged if the queue is empty.
    /// @return true if an event was dequeued, false if the queue is empty.
    bool tryDequeue(SupervisionEvent& event)
    {
        auto item = queue_.tryPop();
        if (!item.has_value())
        {
            return false;
        }
        event = *item;
        return true;
    }

    /// @brief Get Overflow Flag
    /// @details The flag stays set until cleared by a call with reset set to true.
    /// @param [in] reset    If true, atomically clears the flag.
    /// @return The flag value before any reset.
    bool getOverflowFlag(bool reset = false)
    {
        return reset ? overflow_.exchange(false, std::memory_order_relaxed) : overflow_.load(std::memory_order_relaxed);
    }

  private:
    friend struct SupervisionEventQueueTestPeer;

    /// @brief Maps a push result to bool; only kOverflow sets the sticky flag.
    static bool mapEnqueueResult(const score::Result<void>& result, std::atomic<bool>& overflow) noexcept
    {
        if (result.has_value())
        {
            return true;
        }
        if (result.error() == internal::ConcurrencyErrc::kOverflow)
        {
            overflow.store(true, std::memory_order_relaxed);
        }
        return false;
    }

    score::mw::lifecycle::internal::MpscBoundedQueue<SupervisionEvent> queue_;
    std::atomic<bool> overflow_{false};
};

using SupervisionBufferType = SupervisionEventQueue;

}  // namespace score::mw::lifecycle

#endif  // SUPERVISION_EVENT_QUEUE_HPP
