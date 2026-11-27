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

#ifndef SCORE_HEALTH_MONITOR_INTERNAL_MONITORING_LOGIC_H
#define SCORE_HEALTH_MONITOR_INTERNAL_MONITORING_LOGIC_H

#include "score/health_monitor/src/cpp/internal/monitor_evaluator.h"
#include "score/health_monitor/supervisor_client.h"
#include "score/os/time.h"

#include <score/span.hpp>

#include <chrono>

namespace score::health_monitor::details
{

/// @brief One cycle of the worker: evaluates all monitors and notifies the supervisor while none fails.
class MonitoringLogic final
{
  public:
    /// @brief Reads `CLOCK_MONOTONIC` through `time` as the time of the last alive notification.
    /// @param evaluators Monitors to evaluate. The view and each monitor must outlive the logic.
    /// @param client Connected client. It must outlive the logic.
    /// @pre `time` outlives the logic.
    /// @pre `supervisor_api_cycle` fits in `steady_clock::duration`, as `HealthMonitorImpl::Create()` checks.
    MonitoringLogic(
        const score::cpp::span<MonitorEvaluator* const> evaluators,
        const std::chrono::milliseconds supervisor_api_cycle,
        SupervisorClient& client,
        const score::os::Time& time) noexcept;

    MonitoringLogic(const MonitoringLogic&) = delete;
    MonitoringLogic(MonitoringLogic&&) = delete;
    ~MonitoringLogic() = default;
    MonitoringLogic& operator=(const MonitoringLogic&) = delete;
    MonitoringLogic& operator=(MonitoringLogic&&) = delete;

    /// @brief Evaluates every monitor, then notifies alive if no monitor failed and a supervisor cycle passed.
    /// @return `false` if a monitor failed. The worker then stops, so no later notification follows.
    /// @note A notification needs strictly more than one supervisor cycle since the last one.
    bool Run(const std::chrono::steady_clock::time_point hmon_start) noexcept;

  private:
    score::cpp::span<MonitorEvaluator* const> evaluators_;
    std::chrono::milliseconds supervisor_api_cycle_;
    SupervisorClient& client_;
    /// @brief OS interface for clock reads, declared before `last_notification_`, whose initializer reads it.
    const score::os::Time& time_;
    /// @brief Time of the last alive notification, or of the construction before the first one.
    std::chrono::steady_clock::time_point last_notification_;
};

}  // namespace score::health_monitor::details

#endif  // SCORE_HEALTH_MONITOR_INTERNAL_MONITORING_LOGIC_H
