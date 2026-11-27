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

#ifndef SCORE_HEALTH_MONITOR_HEARTBEAT_MONITOR_HEARTBEAT_STATE_H
#define SCORE_HEALTH_MONITOR_HEARTBEAT_MONITOR_HEARTBEAT_STATE_H

#include "score/health_monitor/src/cpp/internal/fetch_update.h"

#include <score/assert.hpp>
#include <score/expected.hpp>

#include <atomic>
#include <cstdint>
#include <optional>

namespace score::health_monitor::details
{

/// @brief Exclusive upper bound of a heartbeat timestamp, which has 62 bits.
const std::uint64_t kHeartbeatTimestampLimit{static_cast<std::uint64_t>(1U) << 62U};

/// @brief Largest heartbeat count: the count has 2 bits.
const std::uint8_t kHeartbeatCounterMax{3U};

/// @brief Bits of the packed value that hold the count.
const std::uint64_t kHeartbeatCounterMask{0b11U};

/// @brief Position of the timestamp in the packed value.
const std::uint64_t kHeartbeatTimestampShift{2U};

/// @brief Time of the last heartbeat and number of heartbeats in one cycle, packed into one 64-bit value.
/// @note The timestamp takes the upper 62 bits and the count the lower 2 bits.
class HeartbeatStateSnapshot final
{
  public:
    /// @brief Creates a snapshot with timestamp 0 and count 0.
    HeartbeatStateSnapshot() noexcept = default;

    /// @brief Creates a snapshot from its packed value.
    explicit HeartbeatStateSnapshot(const std::uint64_t raw) noexcept : raw_{raw}
    {
    }

    /// @brief Returns the packed value.
    std::uint64_t Raw() const noexcept
    {
        return raw_;
    }

    /// @brief Returns the time of the last heartbeat in milliseconds after the creation of the monitor.
    std::uint64_t HeartbeatTimestamp() const noexcept
    {
        return raw_ >> kHeartbeatTimestampShift;
    }

    /// @brief Replaces the timestamp and keeps the count.
    /// @pre `timestamp` is less than `kHeartbeatTimestampLimit`.
    void SetHeartbeatTimestamp(const std::uint64_t timestamp) noexcept
    {
        SCORE_LANGUAGE_FUTURECPP_PRECONDITION(timestamp < kHeartbeatTimestampLimit);
        raw_ = (timestamp << kHeartbeatTimestampShift) | (raw_ & kHeartbeatCounterMask);
    }

    /// @brief Returns the number of heartbeats, at most `kHeartbeatCounterMax`.
    std::uint8_t Counter() const noexcept
    {
        return static_cast<std::uint8_t>(raw_ & kHeartbeatCounterMask);
    }

    /// @brief Adds one to the count and keeps the timestamp.
    /// @note The count stops at `kHeartbeatCounterMax`.
    void IncrementCounter() noexcept
    {
        const std::uint64_t counter{raw_ & kHeartbeatCounterMask};
        const std::uint64_t counter_max{kHeartbeatCounterMax};
        if (counter < counter_max)
        {
            raw_ = (raw_ & ~kHeartbeatCounterMask) | (counter + 1U);
        }
    }

  private:
    /// @brief Timestamp shifted left by 2, combined with the count by bitwise or.
    std::uint64_t raw_{0U};
};

/// @brief Heartbeat state of the current cycle, shared by the threads that send heartbeats and the worker thread.
class HeartbeatState final
{
  public:
    /// @brief Replaces the state with `update(state)` in one atomic step.
    /// @tparam Function Callable from `HeartbeatStateSnapshot` to `std::optional<HeartbeatStateSnapshot>`.
    /// @return The state before the store, or as error the last state for which `update` returns no value.
    /// @warning `update` can run more than once, so it must not change any state other than its result.
    /// @note Any thread can call it, including while `Reset()` runs.
    template <typename Function>
    score::cpp::expected<HeartbeatStateSnapshot, HeartbeatStateSnapshot> Update(Function update) noexcept
    {
        const score::cpp::expected<std::uint64_t, std::uint64_t> result{FetchUpdate(
            state_, std::memory_order_acq_rel, std::memory_order_acquire, [&update](const std::uint64_t previous) {
                const std::optional<HeartbeatStateSnapshot> next{update(HeartbeatStateSnapshot{previous})};
                return next.has_value() ? std::optional<std::uint64_t>{next->Raw()} : std::nullopt;
            })};
        if (!result.has_value())
        {
            return score::cpp::make_unexpected(HeartbeatStateSnapshot{result.error()});
        }

        return HeartbeatStateSnapshot{result.value()};
    }

    /// @brief Returns the state of the ending cycle and resets the state to timestamp 0 and count 0.
    /// @note A heartbeat goes either into the returned snapshot or into the new cycle, never into both.
    HeartbeatStateSnapshot Reset() noexcept
    {
        return HeartbeatStateSnapshot{state_.exchange(0U, std::memory_order_acq_rel)};
    }

  private:
    /// @brief Packed value of a `HeartbeatStateSnapshot`.
    std::atomic<std::uint64_t> state_{0U};
};

static_assert(std::atomic<std::uint64_t>::is_always_lock_free, "Heartbeat() must not take a lock.");

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_HEARTBEAT_MONITOR_HEARTBEAT_STATE_H
