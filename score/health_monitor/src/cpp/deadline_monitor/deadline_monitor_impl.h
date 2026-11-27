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

#ifndef SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_DEADLINE_MONITOR_IMPL_H
#define SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_DEADLINE_MONITOR_IMPL_H

#include "score/containers/non_relocatable_vector.h"
#include "score/health_monitor/deadline.h"
#include "score/health_monitor/deadline_monitor.h"
#include "score/health_monitor/src/cpp/deadline_monitor/deadline_impl.h"
#include "score/health_monitor/src/cpp/deadline_monitor/deadline_state.h"
#include "score/health_monitor/src/cpp/internal/monitor_evaluator.h"
#include "score/health_monitor/tag.h"
#include "score/health_monitor/taken.h"
#include "score/health_monitor/time_range.h"
#include "score/os/time.h"
#include "score/result/result.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <vector>

namespace score::health_monitor::details
{

struct DeadlineConfig
{
    DeadlineTag tag;
    TimeRange range;
};

class DeadlineMonitorImpl final : public DeadlineMonitor,
                                  public MonitorEvaluator,
                                  public std::enable_shared_from_this<DeadlineMonitorImpl>
{
  public:
    DeadlineMonitorImpl(const DeadlineMonitorImpl&) = delete;
    DeadlineMonitorImpl(DeadlineMonitorImpl&&) = delete;
    ~DeadlineMonitorImpl() override = default;
    DeadlineMonitorImpl& operator=(const DeadlineMonitorImpl&) = delete;
    DeadlineMonitorImpl& operator=(DeadlineMonitorImpl&&) = delete;

    /// @brief Creates a deadline monitor and reads `CLOCK_MONOTONIC` through `time` as its creation time.
    /// @return The monitor, or `ErrorCode::kInvalidArgument` if `deadlines` is empty, a deadline tag repeats,
    /// or a range maximum is above `std::numeric_limits<std::uint32_t>::max()` milliseconds.
    /// @pre `time` outlives the monitor.
    static score::Result<std::shared_ptr<DeadlineMonitorImpl>>
    Create(const MonitorTag& tag, const std::vector<DeadlineConfig>& deadlines, const score::os::Time& time) noexcept;

    /// @note The returned deadline shares ownership of this monitor, so the call allocates nothing.
    score::Result<Taken<Deadline>> GetDeadline(const DeadlineTag& tag) noexcept override;

    /// @brief Logs each deadline that stopped too early or runs past its maximum, with the monitor tag.
    /// @param hmon_start Not used: a deadline is measured from its own start.
    /// @return `false` if any deadline failed.
    /// @pre Only the worker thread calls it.
    /// @note It reads the clock before each deadline state, so a deadline stopped in time never counts as missed.
    bool Evaluate(const std::chrono::steady_clock::time_point hmon_start) noexcept override;

  private:
    /// @brief Configured deadline, its deadline object, and whether a handle holds it.
    struct DeadlineTemplate final : public ReleaseHook
    {
        /// @pre `state` and `time` outlive the template.
        DeadlineTemplate(
            const MonitorTag& monitor_tag,
            const DeadlineConfig& config,
            DeadlineState& state,
            const score::os::Time& time,
            const std::chrono::steady_clock::time_point monitor_start) noexcept;

        /// @brief Marks the deadline as held.
        /// @return `false` if a handle already holds the deadline.
        bool TryAcquire() noexcept;

        /// @brief Marks the deadline as free.
        void OnRelease() noexcept override;

        DeadlineTag tag;
        std::atomic<bool> in_use;
        DeadlineImpl deadline;
    };

    /// @brief State of one deadline, which the worker evaluates and which keeps a failure across takes.
    struct ActiveDeadline
    {
        explicit ActiveDeadline(const DeadlineTag& deadline_tag) noexcept;

        DeadlineTag tag;
        DeadlineState state;
    };

    DeadlineMonitorImpl(
        const MonitorTag& tag,
        const std::vector<DeadlineConfig>& deadlines,
        const score::os::Time& time) noexcept;

    /// @brief Tag logged with each failure.
    MonitorTag tag_;
    /// @brief OS interface for clock reads, declared before `monitor_start_` because the constructor reads it first.
    const score::os::Time& time_;
    /// @brief Creation time, the zero point of every timestamp of this monitor.
    std::chrono::steady_clock::time_point monitor_start_;
    /// @brief Deadlines in the order of the configuration.
    score::containers::NonRelocatableVector<DeadlineTemplate> templates_;
    /// @brief States in the order of the configuration, evaluated in that order.
    score::containers::NonRelocatableVector<ActiveDeadline> active_deadlines_;
};

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_DEADLINE_MONITOR_DEADLINE_MONITOR_IMPL_H
