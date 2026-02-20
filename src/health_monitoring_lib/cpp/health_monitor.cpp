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
#include "score/hm/health_monitor.h"

extern "C" {
using namespace score::hm;

// Health Monitor Foreign Function Interface Declarations that are exported by Rust implementation library
internal::FFIHandle health_monitor_builder_create();
void health_monitor_builder_destroy(internal::FFIHandle handler);

internal::FFIHandle health_monitor_builder_build(internal::FFIHandle health_monitor_builder_handle,
                                                 uint32_t supervisor_cycle_ms,
                                                 uint32_t internal_cycle_ms);
void health_monitor_builder_add_deadline_monitor(internal::FFIHandle handle,
                                                 const MonitorTag* tag,
                                                 internal::FFIHandle monitor_handle);
void health_monitor_builder_add_heartbeat_monitor(internal::FFIHandle hmon_builder_handle,
                                                  const MonitorTag* monitor_tag,
                                                  internal::FFIHandle monitor_builder_handle);

internal::FFIHandle health_monitor_get_deadline_monitor(internal::FFIHandle health_monitor_handle,
                                                        const MonitorTag* tag);
internal::FFIHandle health_monitor_get_heartbeat_monitor(internal::FFIHandle hmon_handle,
                                                         const MonitorTag* monitor_tag);
void health_monitor_start(internal::FFIHandle health_monitor_handle);
void health_monitor_destroy(internal::FFIHandle handler);
}

// C++ wrapper for Rust library - the API implementation obeys the Rust API semantics and it's invariants

namespace score::hm
{

HealthMonitorBuilder::HealthMonitorBuilder()
    : health_monitor_builder_handle_{health_monitor_builder_create(), &health_monitor_builder_destroy}
{
}

HealthMonitorBuilder HealthMonitorBuilder::add_deadline_monitor(const MonitorTag& tag,
                                                                deadline::DeadlineMonitorBuilder&& monitor) &&
{
    auto monitor_handle = monitor.drop_by_rust();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(health_monitor_builder_handle_.as_rust_handle().has_value());

    health_monitor_builder_add_deadline_monitor(
        health_monitor_builder_handle_.as_rust_handle().value(), &tag, monitor_handle.value());
    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::add_heartbeat_monitor(const MonitorTag& monitor_tag,
                                                                 heartbeat::HeartbeatMonitorBuilder&& monitor) &&
{
    auto monitor_handle{monitor.drop_by_rust()};
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(health_monitor_builder_handle_.as_rust_handle().has_value());

    health_monitor_builder_add_heartbeat_monitor(
        health_monitor_builder_handle_.as_rust_handle().value(), &monitor_tag, monitor_handle.value());
    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::with_internal_processing_cycle(std::chrono::milliseconds cycle_duration) &&
{
    internal_processing_cycle_duration_ = cycle_duration;
    return std::move(*this);
}

HealthMonitorBuilder HealthMonitorBuilder::with_supervisor_api_cycle(std::chrono::milliseconds cycle_duration) &&
{
    supervisor_api_cycle_duration_ = cycle_duration;
    return std::move(*this);
}

HealthMonitor HealthMonitorBuilder::build() &&
{
    auto handle = health_monitor_builder_handle_.drop_by_rust();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(handle.has_value());

    uint32_t supervisor_duration_ms = static_cast<uint32_t>(supervisor_api_cycle_duration_.count());
    uint32_t internal_duration_ms = static_cast<uint32_t>(internal_processing_cycle_duration_.count());

    return HealthMonitor(health_monitor_builder_build(handle.value(), supervisor_duration_ms, internal_duration_ms));
}

HealthMonitor::HealthMonitor(internal::FFIHandle handle) : health_monitor_(handle)
{
    // Initialize health monitor
}

HealthMonitor::HealthMonitor(HealthMonitor&& other)
{
    health_monitor_ = std::move(other.health_monitor_);
    other.health_monitor_ = nullptr;
}

score::cpp::expected<deadline::DeadlineMonitor, Error> HealthMonitor::get_deadline_monitor(const MonitorTag& tag)
{
    auto maybe_monitor = health_monitor_get_deadline_monitor(health_monitor_, &tag);

    if (maybe_monitor != nullptr)
    {

        return score::cpp::expected<deadline::DeadlineMonitor, Error>(deadline::DeadlineMonitor{maybe_monitor});
    }

    return score::cpp::unexpected(Error::NotFound);
}

score::cpp::expected<heartbeat::HeartbeatMonitor, Error> HealthMonitor::get_heartbeat_monitor(
    const MonitorTag& monitor_tag)
{
    auto maybe_monitor{health_monitor_get_heartbeat_monitor(health_monitor_, &monitor_tag)};
    if (maybe_monitor == nullptr)
    {
        return score::cpp::unexpected(Error::NotFound);
    }
    return score::cpp::expected<heartbeat::HeartbeatMonitor, Error>{heartbeat::HeartbeatMonitor{maybe_monitor}};
}

void HealthMonitor::start()
{
    health_monitor_start(health_monitor_);
}

HealthMonitor::~HealthMonitor()
{
    if (health_monitor_ != nullptr)
    {
        health_monitor_destroy(health_monitor_);
    }
}

HealthMonitor& HealthMonitor::operator=(HealthMonitor&& other)
{
    if (this != &other)
    {
        if (health_monitor_ != nullptr)
        {
            health_monitor_destroy(health_monitor_);
        }
        health_monitor_ = std::move(other.health_monitor_);
        other.health_monitor_ = nullptr;
    }
    return *this;
}

}  // namespace score::hm
