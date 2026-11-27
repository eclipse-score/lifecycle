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

#ifndef SCORE_HEALTH_MONITOR_HEARTBEAT_MONITOR_BUILDER_H
#define SCORE_HEALTH_MONITOR_HEARTBEAT_MONITOR_BUILDER_H

#include "score/health_monitor/time_range.h"

namespace score::health_monitor
{

class HealthMonitorBuilder;

/// @brief Configuration of one heartbeat monitor, given to `HealthMonitorBuilder::AddHeartbeatMonitor()`.
class HeartbeatMonitorBuilder final
{
  public:
    /// @brief Creates the configuration of a heartbeat monitor.
    /// @param range Allowed time between two heartbeats.
    /// @note `HealthMonitorBuilder::Build()` rejects a range whose minimum is at most half the internal cycle.
    explicit HeartbeatMonitorBuilder(const TimeRange& range) noexcept : range_{range}
    {
    }

  private:
    friend class HealthMonitorBuilder;

    /// @brief Allowed time between two heartbeats.
    TimeRange range_;
};

}  // namespace score::health_monitor

#endif  // SCORE_HEALTH_MONITOR_HEARTBEAT_MONITOR_BUILDER_H
