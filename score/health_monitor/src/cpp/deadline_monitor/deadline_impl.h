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

#ifndef SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_DEADLINE_IMPL_H
#define SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_DEADLINE_IMPL_H

#include "score/health_monitor/deadline.h"
#include "score/health_monitor/src/cpp/deadline_monitor/deadline_state.h"
#include "score/health_monitor/tag.h"
#include "score/health_monitor/time_range.h"
#include "score/os/time.h"
#include "score/result/result.h"

#include <chrono>
#include <cstdint>

namespace score::health_monitor::details
{

/// @brief Deadline that records its start and stop in a state that the monitor keeps.
/// @note The deadline monitor reads the state on the worker thread.
class DeadlineImpl final : public Deadline
{
  public:
    /// @param state State of the deadline, which keeps a failure across takes.
    /// @param monitor_start Creation time of the monitor, the zero point of every timestamp of the deadline.
    /// @pre `range.Max()` is at most `std::numeric_limits<std::uint32_t>::max()` milliseconds.
    /// @pre `state` and `time` outlive the deadline.
    DeadlineImpl(
        const MonitorTag& monitor_tag,
        const DeadlineTag& tag,
        const TimeRange& range,
        DeadlineState& state,
        const score::os::Time& time,
        const std::chrono::steady_clock::time_point monitor_start) noexcept;

    DeadlineImpl(const DeadlineImpl&) = delete;
    DeadlineImpl(DeadlineImpl&&) = delete;
    ~DeadlineImpl() override = default;
    DeadlineImpl& operator=(const DeadlineImpl&) = delete;
    DeadlineImpl& operator=(DeadlineImpl&&) = delete;

  private:
    /// @note A rejected start logs a warning after it leaves the state unchanged.
    /// The process aborts if the expected end is later than 2^32 - 1 ms after the creation of the monitor.
    score::Result<void> DoStart() noexcept override;

    /// @note A too-early stop sets the too-early flag before it logs an error.
    /// A late stop leaves the state running, and the worker reports it.
    void DoStop() noexcept override;

    /// @brief Returns the earliest allowed stop of a deadline that runs until `expected`.
    /// @pre `expected` is the timestamp that `DoStart()` stored.
    std::uint32_t EarliestStopMs(const std::uint32_t expected) const noexcept;

    /// @brief Tag of the monitor, logged with each failure.
    MonitorTag monitor_tag_;
    DeadlineTag tag_;
    /// @brief Shortest allowed time from start to stop in milliseconds.
    std::uint32_t range_min_ms_;
    /// @brief Longest allowed time from start to stop in milliseconds.
    std::uint32_t range_max_ms_;
    DeadlineState& state_;
    const score::os::Time& time_;
    std::chrono::steady_clock::time_point monitor_start_;
};

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_DEADLINE_IMPL_H
