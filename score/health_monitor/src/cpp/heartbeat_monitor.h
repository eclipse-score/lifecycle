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

#ifndef SCORE_HEALTH_MONITOR_HEARTBEAT_MONITOR_H
#define SCORE_HEALTH_MONITOR_HEARTBEAT_MONITOR_H

namespace score::health_monitor
{

/// @brief Checks that the time between two heartbeats is in the range given to `HeartbeatMonitorBuilder`.
/// @note The worker thread checks the heartbeats once per internal processing cycle.
/// More than one heartbeat in one cycle is a failure.
class HeartbeatMonitor
{
  public:
    HeartbeatMonitor(const HeartbeatMonitor&) = delete;
    HeartbeatMonitor(HeartbeatMonitor&&) = delete;
    virtual ~HeartbeatMonitor() = default;
    HeartbeatMonitor& operator=(const HeartbeatMonitor&) = delete;
    HeartbeatMonitor& operator=(HeartbeatMonitor&&) = delete;

    /// @brief Records a heartbeat at the current time.
    /// @note Any thread can call it concurrently, also with the worker thread.
    /// Heartbeats recorded before `HealthMonitor::Start()` count in the first cycle.
    virtual void Heartbeat() noexcept = 0;

  protected:
    HeartbeatMonitor() = default;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_HEARTBEAT_MONITOR_H
