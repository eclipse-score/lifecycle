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

#include "score/mw/health/common.h"
#include "score/mw/health/deadline_monitor.h"
#include "score/mw/health/heartbeat_monitor.h"
#include "score/mw/health/logic_monitor.h"
#include "score/mw/health/tag.h"
#include "score/mw/health/thread.h"
#include <utility>

namespace score::mw::health
{

class HealthMonitor;

///
/// Builder for HealthMonitor instances.
///
class HealthMonitorBuilder final
{
  public:
    /// Create a new `HealthMonitorBuilder`.
    HealthMonitorBuilder();

    ~HealthMonitorBuilder() = default;
    HealthMonitorBuilder(const HealthMonitorBuilder&) = delete;
    HealthMonitorBuilder& operator=(const HealthMonitorBuilder&) = delete;

    HealthMonitorBuilder(HealthMonitorBuilder&&) = default;
    HealthMonitorBuilder& operator=(HealthMonitorBuilder&&) = delete;

    /// Adds a deadline monitor to the builder to construct DeadlineMonitor instances during HealthMonitor build.
    HealthMonitorBuilder AddDeadlineMonitor(
        const MonitorTag& monitor_tag,
        deadline::DeadlineMonitorBuilder&& monitor) &&;

    /// Adds a heartbeat monitor for a specific identifier tag.
    HealthMonitorBuilder AddHeartbeatMonitor(
        const MonitorTag& monitor_tag,
        heartbeat::HeartbeatMonitorBuilder&& monitor) &&;

    /// Adds a logic monitor for a specific identifier tag.
    HealthMonitorBuilder AddLogicMonitor(const MonitorTag& monitor_tag, logic::LogicMonitorBuilder&& monitor) &&;

    /// Sets the cycle duration for supervisor API notifications.
    /// This duration determines how often the health monitor notifies the supervisor that the system is alive.
    HealthMonitorBuilder WithSupervisorApiCycle(std::chrono::milliseconds cycle_duration) &&;

    /// Sets the internal processing cycle duration.
    /// This duration determines how often the health monitor checks deadlines.
    HealthMonitorBuilder WithInternalProcessingCycle(std::chrono::milliseconds cycle_duration) &&;

    /// Sets the monitoring thread parameters.
    HealthMonitorBuilder WithThreadParameters(score::mw::health::ThreadParameters&& thread_parameters) &&;

    /// Build a new `HealthMonitor` instance based on provided parameters.
    score::cpp::expected<HealthMonitor, Error> Build() &&;

    /// @deprecated Use `AddDeadlineMonitor()` instead. Removed in the release after v0.10.
    [[deprecated("Use AddDeadlineMonitor() instead. The snake_case API is removed in the release after v0.10.")]]
    HealthMonitorBuilder add_deadline_monitor(
        const MonitorTag& monitor_tag,
        deadline::DeadlineMonitorBuilder&& monitor) &&;

    /// @deprecated Use `AddHeartbeatMonitor()` instead. Removed in the release after v0.10.
    [[deprecated("Use AddHeartbeatMonitor() instead. The snake_case API is removed in the release after v0.10.")]]
    HealthMonitorBuilder add_heartbeat_monitor(
        const MonitorTag& monitor_tag,
        heartbeat::HeartbeatMonitorBuilder&& monitor) &&;

    /// @deprecated Use `AddLogicMonitor()` instead. Removed in the release after v0.10.
    [[deprecated("Use AddLogicMonitor() instead. The snake_case API is removed in the release after v0.10.")]]
    HealthMonitorBuilder add_logic_monitor(const MonitorTag& monitor_tag, logic::LogicMonitorBuilder&& monitor) &&;

    /// @deprecated Use `WithSupervisorApiCycle()` instead. Removed in the release after v0.10.
    [[deprecated("Use WithSupervisorApiCycle() instead. The snake_case API is removed in the release after v0.10.")]]
    HealthMonitorBuilder with_supervisor_api_cycle(std::chrono::milliseconds cycle_duration) &&;

    /// @deprecated Use `WithInternalProcessingCycle()` instead. Removed in the release after v0.10.
    [[deprecated(
        "Use WithInternalProcessingCycle() instead. The snake_case API is removed in the release after v0.10.")]]
    HealthMonitorBuilder with_internal_processing_cycle(std::chrono::milliseconds cycle_duration) &&;

    /// @deprecated Use `WithThreadParameters()` instead. Removed in the release after v0.10.
    [[deprecated("Use WithThreadParameters() instead. The snake_case API is removed in the release after v0.10.")]]
    HealthMonitorBuilder thread_parameters(score::mw::health::ThreadParameters&& thread_parameters) &&;

    /// @deprecated Use `Build()` instead. Removed in the release after v0.10.
    [[deprecated("Use Build() instead. The snake_case API is removed in the release after v0.10.")]]
    score::cpp::expected<HealthMonitor, Error> build() &&;

  private:
    internal::DroppableFFIHandle health_monitor_builder_handle_;

    std::optional<uint64_t> supervisor_api_cycle_ms_;
    std::optional<uint64_t> internal_processing_cycle_ms_;
    std::optional<ThreadParameters> thread_parameters_;
};

class HealthMonitor final
{
  public:
    HealthMonitor(const HealthMonitor&) = delete;
    HealthMonitor& operator=(const HealthMonitor&) = delete;

    HealthMonitor(HealthMonitor&& other);
    HealthMonitor& operator=(HealthMonitor&&);

    ~HealthMonitor();

    score::cpp::expected<deadline::DeadlineMonitor, Error> GetDeadlineMonitor(const MonitorTag& monitor_tag);
    score::cpp::expected<heartbeat::HeartbeatMonitor, Error> GetHeartbeatMonitor(const MonitorTag& monitor_tag);
    score::cpp::expected<logic::LogicMonitor, Error> GetLogicMonitor(const MonitorTag& monitor_tag);

    void Start();

    /// @deprecated Use `GetDeadlineMonitor()` instead. Removed in the release after v0.10.
    [[deprecated("Use GetDeadlineMonitor() instead. The snake_case API is removed in the release after v0.10.")]]
    score::cpp::expected<deadline::DeadlineMonitor, Error> get_deadline_monitor(const MonitorTag& monitor_tag);

    /// @deprecated Use `GetHeartbeatMonitor()` instead. Removed in the release after v0.10.
    [[deprecated("Use GetHeartbeatMonitor() instead. The snake_case API is removed in the release after v0.10.")]]
    score::cpp::expected<heartbeat::HeartbeatMonitor, Error> get_heartbeat_monitor(const MonitorTag& monitor_tag);

    /// @deprecated Use `GetLogicMonitor()` instead. Removed in the release after v0.10.
    [[deprecated("Use GetLogicMonitor() instead. The snake_case API is removed in the release after v0.10.")]]
    score::cpp::expected<logic::LogicMonitor, Error> get_logic_monitor(const MonitorTag& monitor_tag);

    /// @deprecated Use `Start()` instead. Removed in the release after v0.10.
    [[deprecated("Use Start() instead. The snake_case API is removed in the release after v0.10.")]]
    void start();

  private:
    // Allow only the builder to create HealthMonitor instances.
    friend class HealthMonitorBuilder;

    HealthMonitor(internal::FFIHandle handle);

    internal::FFIHandle health_monitor_;
};

// Deprecated snake_case API. Each entry forwards to its CamelCase replacement and is removed in the release after
// v0.10. Defined out-of-line so the forwarded return types are complete at the point of definition.

inline HealthMonitorBuilder HealthMonitorBuilder::add_deadline_monitor(
    const MonitorTag& monitor_tag,
    deadline::DeadlineMonitorBuilder&& monitor) &&
{
    return std::move(*this).AddDeadlineMonitor(monitor_tag, std::move(monitor));
}

inline HealthMonitorBuilder HealthMonitorBuilder::add_heartbeat_monitor(
    const MonitorTag& monitor_tag,
    heartbeat::HeartbeatMonitorBuilder&& monitor) &&
{
    return std::move(*this).AddHeartbeatMonitor(monitor_tag, std::move(monitor));
}

inline HealthMonitorBuilder HealthMonitorBuilder::add_logic_monitor(
    const MonitorTag& monitor_tag,
    logic::LogicMonitorBuilder&& monitor) &&
{
    return std::move(*this).AddLogicMonitor(monitor_tag, std::move(monitor));
}

inline HealthMonitorBuilder HealthMonitorBuilder::with_supervisor_api_cycle(std::chrono::milliseconds cycle_duration) &&
{
    return std::move(*this).WithSupervisorApiCycle(cycle_duration);
}

inline HealthMonitorBuilder HealthMonitorBuilder::with_internal_processing_cycle(
    std::chrono::milliseconds cycle_duration) &&
{
    return std::move(*this).WithInternalProcessingCycle(cycle_duration);
}

inline HealthMonitorBuilder HealthMonitorBuilder::thread_parameters(
    score::mw::health::ThreadParameters&& thread_parameters) &&
{
    return std::move(*this).WithThreadParameters(std::move(thread_parameters));
}

inline score::cpp::expected<HealthMonitor, Error> HealthMonitorBuilder::build() &&
{
    return std::move(*this).Build();
}

inline score::cpp::expected<deadline::DeadlineMonitor, Error> HealthMonitor::get_deadline_monitor(
    const MonitorTag& monitor_tag)
{
    return GetDeadlineMonitor(monitor_tag);
}

inline score::cpp::expected<heartbeat::HeartbeatMonitor, Error> HealthMonitor::get_heartbeat_monitor(
    const MonitorTag& monitor_tag)
{
    return GetHeartbeatMonitor(monitor_tag);
}

inline score::cpp::expected<logic::LogicMonitor, Error> HealthMonitor::get_logic_monitor(const MonitorTag& monitor_tag)
{
    return GetLogicMonitor(monitor_tag);
}

inline void HealthMonitor::start()
{
    Start();
}

}  // namespace score::mw::health

#endif  // SCORE_HM_HEALTH_MONITOR_H
