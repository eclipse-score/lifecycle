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
#ifndef SCORE_HM_HEALTH_MONITOR_H
#define SCORE_HM_HEALTH_MONITOR_H

#include <score/hm/common.h>
#include <score/hm/deadline/deadline_monitor.h>
#include <score/hm/heartbeat/heartbeat_monitor.h>
#include <score/hm/tag.h>

namespace score::hm
{

class HealthMonitor;

///
/// Builder for HealthMonitor instances.
///
class HealthMonitorBuilder final
{
  public:
    ///  Creates a new HealthMonitorBuilder
    HealthMonitorBuilder();

    ~HealthMonitorBuilder() = default;
    HealthMonitorBuilder(const HealthMonitorBuilder&) = delete;
    HealthMonitorBuilder& operator=(const HealthMonitorBuilder&) = delete;

    HealthMonitorBuilder(HealthMonitorBuilder&&) = default;
    HealthMonitorBuilder& operator=(HealthMonitorBuilder&&) = delete;

    /// Adds a deadline monitor to the builder to construct DeadlineMonitor instances during HealthMonitor build.
    HealthMonitorBuilder add_deadline_monitor(const MonitorTag& tag, deadline::DeadlineMonitorBuilder&& monitor) &&;

    /// Adds a heartbeat monitor for a specific identifier tag.
    HealthMonitorBuilder add_heartbeat_monitor(const MonitorTag& tag, heartbeat::HeartbeatMonitorBuilder&& monitor) &&;

    /// Sets the cycle duration for supervisor API notifications.
    /// This duration determines how often the health monitor notifies the supervisor that the system is alive.
    HealthMonitorBuilder with_supervisor_api_cycle(std::chrono::milliseconds cycle_duration) &&;

    /// Sets the internal processing cycle duration.
    /// This duration determines how often the health monitor checks deadlines.
    HealthMonitorBuilder with_internal_processing_cycle(std::chrono::milliseconds cycle_duration) &&;

    /// Builds and returns the HealthMonitor instance.
    HealthMonitor build() &&;

  private:
    internal::DroppableFFIHandle health_monitor_builder_handle_;

    std::chrono::milliseconds supervisor_api_cycle_duration_;
    std::chrono::milliseconds internal_processing_cycle_duration_;
};

class HealthMonitor final
{
  public:
    HealthMonitor(const HealthMonitor&) = delete;
    HealthMonitor& operator=(const HealthMonitor&) = delete;

    HealthMonitor(HealthMonitor&& other);
    HealthMonitor& operator=(HealthMonitor&&);

    ~HealthMonitor();

    score::cpp::expected<deadline::DeadlineMonitor, Error> get_deadline_monitor(const MonitorTag& tag);
    score::cpp::expected<heartbeat::HeartbeatMonitor, Error> get_heartbeat_monitor(const MonitorTag& monitor_tag);

    void start();

  private:
    // Allow only the builder to create HealthMonitor instances.
    friend class HealthMonitorBuilder;

    HealthMonitor(internal::FFIHandle handle);

    internal::FFIHandle health_monitor_;
};

}  // namespace score::hm

#endif  // SCORE_HM_HEALTH_MONITOR_H
