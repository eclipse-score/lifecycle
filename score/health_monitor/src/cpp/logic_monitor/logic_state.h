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

#ifndef SCORE_HEALTH_MONITOR_LOGIC_MONITOR_LOGIC_STATE_H
#define SCORE_HEALTH_MONITOR_LOGIC_MONITOR_LOGIC_STATE_H

#include <score/assert.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace score::health_monitor::details
{

/// @brief Failure of a logic monitor, latched until the monitor is destroyed.
/// @note The values are the status byte of the packed logic state, where 0 means healthy.
enum class LogicEvaluationError : std::uint8_t
{
    /// @brief The current state cannot be determined.
    kInvalidState = 1U,
    /// @brief A transition targets a state that the current state does not list.
    kInvalidTransition = 2U,
    /// @brief The status byte is non-zero and matches no other enumerator.
    kUnmappedError = 3U,
};

/// @brief Exclusive upper bound of a state index, which has 56 bits.
const std::uint64_t kLogicStateIndexLimit{static_cast<std::uint64_t>(1U) << 56U};

/// @brief Bits of the packed value that hold the status.
const std::uint64_t kLogicStatusMask{0xFFU};

/// @brief Position of the state index in the packed value.
const std::uint64_t kLogicStateIndexShift{8U};

/// @brief Current state index and monitor status of a logic monitor, packed into one 64-bit value.
/// @note The index takes the upper 56 bits and the status the lower 8 bits, where 0 means healthy.
class LogicStateSnapshot final
{
  public:
    /// @brief Creates a healthy snapshot at state `state_index`.
    /// @pre `state_index` is less than `kLogicStateIndexLimit`.
    explicit LogicStateSnapshot(const std::size_t state_index) noexcept
    {
        SetCurrentStateIndex(state_index);
    }

    /// @brief Creates a snapshot from its packed value.
    static LogicStateSnapshot FromRaw(const std::uint64_t raw) noexcept
    {
        LogicStateSnapshot snapshot{0U};
        snapshot.raw_ = raw;
        return snapshot;
    }

    /// @brief Returns the packed value.
    std::uint64_t Raw() const noexcept
    {
        return raw_;
    }

    /// @brief Returns the position of the current state in the state graph.
    std::size_t CurrentStateIndex() const noexcept
    {
        return static_cast<std::size_t>(raw_ >> kLogicStateIndexShift);
    }

    /// @brief Replaces the state index and keeps the status.
    /// @pre `state_index` is less than `kLogicStateIndexLimit`.
    void SetCurrentStateIndex(const std::size_t state_index) noexcept
    {
        const std::uint64_t index{static_cast<std::uint64_t>(state_index)};
        SCORE_LANGUAGE_FUTURECPP_PRECONDITION(index < kLogicStateIndexLimit);
        raw_ = (index << kLogicStateIndexShift) | (raw_ & kLogicStatusMask);
    }

    /// @brief Returns the failure of the monitor, or no value if it is healthy.
    /// @note A status byte that is no `LogicEvaluationError` value reads as `kUnmappedError`.
    std::optional<LogicEvaluationError> MonitorStatus() const noexcept
    {
        const std::uint64_t status{raw_ & kLogicStatusMask};
        if (status == 0U)
        {
            return std::nullopt;
        }

        const std::uint64_t invalid_state{static_cast<std::uint64_t>(LogicEvaluationError::kInvalidState)};
        const std::uint64_t invalid_transition{static_cast<std::uint64_t>(LogicEvaluationError::kInvalidTransition)};
        if (status == invalid_state)
        {
            return LogicEvaluationError::kInvalidState;
        }

        if (status == invalid_transition)
        {
            return LogicEvaluationError::kInvalidTransition;
        }

        return LogicEvaluationError::kUnmappedError;
    }

    /// @brief Replaces the status with `error` and keeps the state index.
    void SetMonitorStatus(const LogicEvaluationError error) noexcept
    {
        raw_ = static_cast<std::uint64_t>(error) | (raw_ & ~kLogicStatusMask);
    }

  private:
    /// @brief State index shifted left by 8, combined with the status by bitwise or.
    std::uint64_t raw_{0U};
};

/// @brief Logic state shared by the thread that reports transitions and the worker thread.
class LogicState final
{
  public:
    /// @brief Creates a healthy state at state `initial_state_index`.
    /// @pre `initial_state_index` is less than `kLogicStateIndexLimit`.
    explicit LogicState(const std::size_t initial_state_index) noexcept
        : state_{LogicStateSnapshot{initial_state_index}.Raw()}
    {
    }

    /// @brief Returns the current state.
    LogicStateSnapshot Snapshot() const noexcept
    {
        return LogicStateSnapshot::FromRaw(state_.load(std::memory_order_acquire));
    }

    /// @brief Stores `next` and returns the state before the store.
    LogicStateSnapshot Swap(const LogicStateSnapshot next) noexcept
    {
        return LogicStateSnapshot::FromRaw(state_.exchange(next.Raw(), std::memory_order_acq_rel));
    }

  private:
    /// @brief Packed value of a `LogicStateSnapshot`.
    std::atomic<std::uint64_t> state_;
};

static_assert(std::atomic<std::uint64_t>::is_always_lock_free, "Transition() must not take a lock.");

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_LOGIC_MONITOR_LOGIC_STATE_H
