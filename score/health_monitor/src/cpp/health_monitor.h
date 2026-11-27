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

#ifndef SCORE_HEALTH_MONITOR_HEALTH_MONITOR_H
#define SCORE_HEALTH_MONITOR_HEALTH_MONITOR_H

#include "score/health_monitor/deadline_monitor.h"
#include "score/health_monitor/heartbeat_monitor.h"
#include "score/health_monitor/logic_monitor.h"
#include "score/health_monitor/tag.h"
#include "score/health_monitor/taken.h"
#include "score/result/result.h"

namespace score::health_monitor
{

/// @brief Owns the worker thread that checks the monitors and notifies the supervisor.
/// @pre The application takes every monitor, then calls `Start()`.
/// @note The class is not thread-safe.
/// @note The worker stops the alive notifications at the first failure, so the supervisor detects it.
/// The destructor stops the worker thread.
/// @note The health monitor shares each monitor with its `Taken` handle.
/// A taken monitor stays valid after the health monitor is destroyed, and nothing checks it then.
class HealthMonitor
{
  public:
    HealthMonitor(const HealthMonitor&) = delete;
    HealthMonitor(HealthMonitor&&) = delete;
    virtual ~HealthMonitor() = default;
    HealthMonitor& operator=(const HealthMonitor&) = delete;
    HealthMonitor& operator=(HealthMonitor&&) = delete;

    /// @brief Returns the deadline monitor with this tag, once per tag.
    /// @return The monitor, valid also after the health monitor is destroyed, or one of these errors:
    /// `ErrorCode::kUnknownTag` if no deadline monitor has this tag.
    /// `ErrorCode::kAlreadyTaken` if an earlier call returned this monitor.
    /// `ErrorCode::kAlreadyStarted` if a `Start()` call succeeded.
    virtual score::Result<Taken<DeadlineMonitor>> GetDeadlineMonitor(const MonitorTag& tag) noexcept = 0;

    /// @brief Returns the heartbeat monitor with this tag, once per tag.
    /// @return The monitor, valid also after the health monitor is destroyed, or one of these errors:
    /// `ErrorCode::kUnknownTag` if no heartbeat monitor has this tag.
    /// `ErrorCode::kAlreadyTaken` if an earlier call returned this monitor.
    /// `ErrorCode::kAlreadyStarted` if a `Start()` call succeeded.
    virtual score::Result<Taken<HeartbeatMonitor>> GetHeartbeatMonitor(const MonitorTag& tag) noexcept = 0;

    /// @brief Returns the logic monitor with this tag, once per tag.
    /// @return The monitor, valid also after the health monitor is destroyed, or one of these errors:
    /// `ErrorCode::kUnknownTag` if no logic monitor has this tag.
    /// `ErrorCode::kAlreadyTaken` if an earlier call returned this monitor.
    /// `ErrorCode::kAlreadyStarted` if a `Start()` call succeeded.
    virtual score::Result<Taken<LogicMonitor>> GetLogicMonitor(const MonitorTag& tag) noexcept = 0;

    /// @brief Connects to the supervisor and starts the worker thread.
    /// @pre The application calls it before it reports the running state to its supervisor.
    /// @pre No thread changes the environment while `Start()` runs, because connecting to the supervisor can read it.
    /// @return An empty result on success, or one of these errors:
    /// `ErrorCode::kNotTaken` if the application did not take a monitor.
    /// `ErrorCode::kSupervisorUnavailable` if the health monitor cannot connect to its supervisor.
    /// `ErrorCode::kAlreadyStarted` if an earlier call started the worker, also when the worker stopped at a failure.
    /// @note After `kNotTaken` or `kSupervisorUnavailable` no worker runs, and a later call can try again.
    /// @note The process terminates if the worker cannot start or cannot apply its `ThreadParameters`.
    /// @note Nothing allocates memory after this call.
    virtual score::Result<void> Start() noexcept = 0;

  protected:
    HealthMonitor() = default;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_HEALTH_MONITOR_H
