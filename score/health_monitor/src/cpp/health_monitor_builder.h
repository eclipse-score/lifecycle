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

#ifndef SCORE_HEALTH_MONITOR_HEALTH_MONITOR_BUILDER_H
#define SCORE_HEALTH_MONITOR_HEALTH_MONITOR_BUILDER_H

#include "score/health_monitor/deadline_monitor_builder.h"
#include "score/health_monitor/health_monitor.h"
#include "score/health_monitor/heartbeat_monitor_builder.h"
#include "score/health_monitor/logic_monitor_builder.h"
#include "score/health_monitor/tag.h"
#include "score/health_monitor/thread_parameters.h"
#include "score/result/result.h"

#include <chrono>
#include <memory>
#include <utility>
#include <vector>

namespace score::health_monitor
{

/// @brief Collects the monitor configuration and creates the health monitor.
/// @note Each monitor needs its own tag, also across the deadline, heartbeat and logic kinds.
class HealthMonitorBuilder final
{
  public:
    /// @brief Adds a deadline monitor under `tag`.
    HealthMonitorBuilder AddDeadlineMonitor(const MonitorTag& tag, DeadlineMonitorBuilder&& monitor) && noexcept;

    /// @brief Adds a heartbeat monitor under `tag`.
    HealthMonitorBuilder AddHeartbeatMonitor(const MonitorTag& tag, HeartbeatMonitorBuilder&& monitor) && noexcept;

    /// @brief Adds a logic monitor under `tag`.
    HealthMonitorBuilder AddLogicMonitor(const MonitorTag& tag, LogicMonitorBuilder&& monitor) && noexcept;

    /// @brief Sets the time between two alive notifications to the supervisor.
    /// @param cycle A positive multiple of the internal processing cycle.
    /// It must not exceed `std::chrono::steady_clock::duration::max()`.
    HealthMonitorBuilder WithSupervisorApiCycle(const std::chrono::milliseconds cycle) && noexcept;

    /// @brief Sets the time between two checks of all monitors.
    /// @param cycle Must be positive and must not exceed `std::chrono::steady_clock::duration::max()`.
    HealthMonitorBuilder WithInternalProcessingCycle(const std::chrono::milliseconds cycle) && noexcept;

    /// @brief Sets the scheduling, CPU affinity and stack size of the worker thread.
    HealthMonitorBuilder WithThreadParameters(ThreadParameters&& thread_parameters) && noexcept;

    /// @brief Creates the health monitor with all its monitors.
    /// @return The health monitor, or one of these errors:
    /// `ErrorCode::kNoMonitors` if no monitor was added.
    /// `ErrorCode::kInvalidArgument` if a tag repeats or a cycle, range, state graph or thread parameter is not valid.
    /// The setter or builder of each input states its rule.
    score::Result<std::unique_ptr<HealthMonitor>> Build() && noexcept;

  private:
    /// @brief Deadline monitors in the order of the `AddDeadlineMonitor()` calls.
    std::vector<std::pair<MonitorTag, DeadlineMonitorBuilder>> deadline_monitors_;
    /// @brief Heartbeat monitors in the order of the `AddHeartbeatMonitor()` calls.
    std::vector<std::pair<MonitorTag, HeartbeatMonitorBuilder>> heartbeat_monitors_;
    /// @brief Logic monitors in the order of the `AddLogicMonitor()` calls.
    std::vector<std::pair<MonitorTag, LogicMonitorBuilder>> logic_monitors_;
    /// @brief Time between two alive notifications to the supervisor.
    std::chrono::milliseconds supervisor_api_cycle_{500};
    /// @brief Time between two checks of all monitors.
    std::chrono::milliseconds internal_processing_cycle_{100};
    /// @brief Scheduling, CPU affinity and stack size of the worker thread.
    ThreadParameters thread_parameters_;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_HEALTH_MONITOR_BUILDER_H
