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

#ifndef SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_DEADLINE_STATE_H
#define SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_DEADLINE_STATE_H

#include "score/health_monitor/src/cpp/internal/fetch_update.h"

#include <score/expected.hpp>

#include <atomic>
#include <cstdint>
#include <optional>

namespace score::health_monitor::details
{

/// @brief Bits of the packed value that hold the flags.
const std::uint64_t kDeadlineFlagsMask{0b1111U};

/// @brief Flag set in every snapshot that starts from the default.
const std::uint64_t kDeadlineStopped{0b0001U};

/// @brief Flag set while a started deadline has not passed its check at stop.
const std::uint64_t kDeadlineRunning{0b0010U};

/// @brief Flag set when a deadline stopped before the minimum of its range.
const std::uint64_t kDeadlineTooEarly{0b1000U};

/// @brief Position of the timestamp in the packed value.
const std::uint64_t kDeadlineTimestampShift{32U};

/// @brief Expected end and flags of one deadline, packed into one 64-bit value.
/// @note The timestamp takes the upper 32 bits, bits 4 to 31 are reserved, and the flags take the lower 4 bits.
class DeadlineStateSnapshot final
{
  public:
    /// @brief Creates a stopped snapshot with timestamp 0.
    DeadlineStateSnapshot() noexcept = default;

    /// @brief Creates a snapshot from its packed value.
    explicit DeadlineStateSnapshot(const std::uint64_t raw) noexcept : raw_{raw}
    {
    }

    std::uint64_t Raw() const noexcept
    {
        return raw_;
    }

    /// @brief Returns true if the deadline started and did not pass its check at stop.
    bool IsRunning() const noexcept
    {
        return (raw_ & kDeadlineRunning) != 0U;
    }

    /// @note A running snapshot built from the default keeps the stopped flag.
    bool IsStopped() const noexcept
    {
        return (raw_ & kDeadlineStopped) != 0U;
    }

    /// @brief Returns true if the deadline stopped before the minimum of its range.
    bool IsTooEarly() const noexcept
    {
        return (raw_ & kDeadlineTooEarly) != 0U;
    }

    /// @brief Returns the expected end of the deadline in milliseconds after the creation of the monitor.
    std::uint32_t TimestampMs() const noexcept
    {
        return static_cast<std::uint32_t>(raw_ >> kDeadlineTimestampShift);
    }

    /// @brief Replaces the timestamp, keeps the flags and clears the reserved bits.
    void SetTimestampMs(const std::uint32_t timestamp) noexcept
    {
        raw_ = (static_cast<std::uint64_t>(timestamp) << kDeadlineTimestampShift) | (raw_ & kDeadlineFlagsMask);
    }

    void SetRunning() noexcept
    {
        raw_ |= kDeadlineRunning;
    }

    void SetTooEarly() noexcept
    {
        raw_ |= kDeadlineTooEarly;
    }

  private:
    std::uint64_t raw_{kDeadlineStopped};
};

/// @brief Deadline state shared by the thread that starts and stops the deadline and the worker thread.
/// @note All accesses are relaxed: the packed value is the only data the threads share.
class DeadlineState final
{
  public:
    DeadlineStateSnapshot Snapshot() const noexcept
    {
        return DeadlineStateSnapshot{state_.load(std::memory_order_relaxed)};
    }

    /// @brief Replaces the state with `update(state)` in one atomic step.
    /// @tparam Function Callable from `DeadlineStateSnapshot` to `std::optional<DeadlineStateSnapshot>`.
    /// @return The state before the store, or as error the last state for which `update` returns no value.
    /// @warning `update` can run more than once, so each run must overwrite every value it writes outside its result.
    template <typename Function>
    score::cpp::expected<DeadlineStateSnapshot, DeadlineStateSnapshot> Update(Function update) noexcept
    {
        const score::cpp::expected<std::uint64_t, std::uint64_t> result{FetchUpdate(
            state_, std::memory_order_relaxed, std::memory_order_relaxed, [&update](const std::uint64_t previous) {
                const std::optional<DeadlineStateSnapshot> next{update(DeadlineStateSnapshot{previous})};
                return next.has_value() ? std::optional<std::uint64_t>{next->Raw()} : std::nullopt;
            })};
        if (!result.has_value())
        {
            return score::cpp::make_unexpected(DeadlineStateSnapshot{result.error()});
        }

        return DeadlineStateSnapshot{result.value()};
    }

  private:
    /// @brief Packed value of a `DeadlineStateSnapshot`.
    std::atomic<std::uint64_t> state_{kDeadlineStopped};
};

static_assert(std::atomic<std::uint64_t>::is_always_lock_free, "Deadline start and stop must not take a lock.");

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_DEADLINE_STATE_H
