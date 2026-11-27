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

#ifndef SCORE_HEALTH_MONITOR_HEARTBEAT_MONITOR_HEARTBEAT_MONITOR_IMPL_H
#define SCORE_HEALTH_MONITOR_HEARTBEAT_MONITOR_HEARTBEAT_MONITOR_IMPL_H

#include "score/health_monitor/heartbeat_monitor.h"
#include "score/health_monitor/src/cpp/heartbeat_monitor/heartbeat_state.h"
#include "score/health_monitor/src/cpp/internal/monitor_evaluator.h"
#include "score/health_monitor/tag.h"
#include "score/health_monitor/time_range.h"
#include "score/os/time.h"
#include "score/result/result.h"

#include <chrono>
#include <cstdint>
#include <memory>

namespace score::health_monitor::details
{

/// @brief Heartbeat monitor that checks the time between two heartbeats against a range.
/// @note A cycle starts at the start of the worker thread, then at each heartbeat in the range.
/// A failed evaluation does not start a new cycle, so the range of the old cycle stays in force.
class HeartbeatMonitorImpl final : public HeartbeatMonitor, public MonitorEvaluator
{
  public:
    HeartbeatMonitorImpl(const HeartbeatMonitorImpl&) = delete;
    HeartbeatMonitorImpl(HeartbeatMonitorImpl&&) = delete;
    ~HeartbeatMonitorImpl() override = default;
    HeartbeatMonitorImpl& operator=(const HeartbeatMonitorImpl&) = delete;
    HeartbeatMonitorImpl& operator=(HeartbeatMonitorImpl&&) = delete;

    /// @brief Creates a heartbeat monitor and reads `CLOCK_MONOTONIC` through `time` as its creation time.
    /// @param range Allowed time between two heartbeats.
    /// @param internal_processing_cycle Time between two calls of `Evaluate()`.
    /// @return The monitor, or `ErrorCode::kInvalidArgument` if the cycle is at least twice `range.Min()`.
    /// @pre `time` outlives the monitor.
    static score::Result<std::shared_ptr<HeartbeatMonitorImpl>> Create(
        const MonitorTag& tag,
        const TimeRange& range,
        const std::chrono::milliseconds internal_processing_cycle,
        const score::os::Time& time) noexcept;

    /// @brief Records a heartbeat at the current time of the clock.
    /// @note Any thread can call it, including while `Evaluate()` runs.
    /// @note It reads the clock before it stores the heartbeat.
    /// A heartbeat that overlaps an evaluation can count in the next cycle with the earlier time.
    void Heartbeat() noexcept override;

    /// @brief Checks the heartbeats since the last call and reports each failure to `sink` with the monitor tag.
    /// @pre `hmon_start` is not earlier than the creation of the monitor.
    /// @pre Only the worker thread calls it.
    void Evaluate(const std::chrono::steady_clock::time_point hmon_start, EvaluationErrorSink& sink) noexcept override;

  private:
    HeartbeatMonitorImpl(const MonitorTag& tag, const TimeRange& range, const score::os::Time& time) noexcept;

    /// @brief Returns the whole milliseconds from `earlier` to `later`, or 0 if `later` is before `earlier`.
    static std::uint64_t ElapsedMs(
        const std::chrono::steady_clock::time_point earlier,
        const std::chrono::steady_clock::time_point later) noexcept;

    /// @brief Tag given to `sink` with each failure.
    MonitorTag tag_;
    /// @brief Shortest allowed time between two heartbeats in milliseconds.
    std::uint64_t range_min_ms_;
    /// @brief Longest allowed time between two heartbeats in milliseconds.
    std::uint64_t range_max_ms_;
    /// @brief OS interface for clock reads, declared before `monitor_start_` because the constructor reads it first.
    const score::os::Time& time_;
    /// @brief Creation time, the zero point of every timestamp of this monitor.
    std::chrono::steady_clock::time_point monitor_start_;
    /// @brief Heartbeats of the current cycle.
    HeartbeatState state_;
    /// @brief Cycle start in milliseconds after `monitor_start_`, or 0 before the first heartbeat in the range.
    /// Only the worker thread reads and writes it.
    std::uint64_t cycle_start_ms_{0U};
};

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_HEARTBEAT_MONITOR_HEARTBEAT_MONITOR_IMPL_H
