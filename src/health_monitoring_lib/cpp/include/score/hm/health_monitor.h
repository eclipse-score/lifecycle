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
    HealthMonitorBuilder add_deadline_monitor(const IdentTag& tag, deadline::DeadlineMonitorBuilder&& monitor) &&;

    /// Builds and returns the HealthMonitor instance.
    HealthMonitor build() &&;

  private:
    internal::DroppableFFIHandle health_monitor_builder_handle_;
};

class HealthMonitor final
{
  public:
    HealthMonitor(const HealthMonitor&) = delete;
    HealthMonitor& operator=(const HealthMonitor&) = delete;

    HealthMonitor(HealthMonitor&& other);
    HealthMonitor& operator=(HealthMonitor&&);

    ~HealthMonitor();

    score::cpp::expected<deadline::DeadlineMonitor, Error> get_deadline_monitor(const IdentTag& tag);

  private:
    // Allow only the builder to create HealthMonitor instances.
    friend class HealthMonitorBuilder;

    HealthMonitor(internal::FFIHandle handle);

    internal::FFIHandle health_monitor_;
};

}  // namespace score::hm

#endif  // SCORE_HM_HEALTH_MONITOR_H
