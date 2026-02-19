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

namespace
{
extern "C" {
using namespace score::hm;
using namespace score::hm::internal;
using namespace score::hm::deadline;

// Functions below must match functions defined in `crate::ffi`.

FFICode health_monitor_builder_create(FFIHandle* health_monitor_builder_handle_out);
FFICode health_monitor_builder_destroy(FFIHandle health_monitor_builder_handle);
FFICode health_monitor_builder_build(FFIHandle health_monitor_builder_handle,
                                     uint32_t supervisor_cycle_ms,
                                     uint32_t internal_cycle_ms,
                                     FFIHandle* health_monitor_handle_out);
FFICode health_monitor_builder_add_deadline_monitor(FFIHandle health_monitor_builder_handle,
                                                    const IdentTag* monitor_tag,
                                                    FFIHandle deadline_monitor_builder_handle);
FFICode health_monitor_get_deadline_monitor(FFIHandle health_monitor_handle,
                                            const IdentTag* monitor_tag,
                                            FFIHandle* deadline_monitor_handle_out);
FFICode health_monitor_start(FFIHandle health_monitor_handle);
FFICode health_monitor_destroy(FFIHandle health_monitor_handle);
}

FFIHandle health_monitor_builder_create_wrapper()
{
    FFIHandle handle{nullptr};
    auto result{health_monitor_builder_create(&handle)};
    abort_on_error(result);
    return handle;
}

}  // namespace

// C++ wrapper for Rust library - the API implementation obeys the Rust API semantics and it's invariants

namespace score::hm
{

HealthMonitorBuilder::HealthMonitorBuilder()
    : health_monitor_builder_handle_{health_monitor_builder_create_wrapper(), &health_monitor_builder_destroy}
{
}

HealthMonitorBuilder HealthMonitorBuilder::add_deadline_monitor(const IdentTag& tag,
                                                                DeadlineMonitorBuilder&& monitor) &&
{
    auto monitor_handle = monitor.drop_by_rust();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(health_monitor_builder_handle_.as_rust_handle().has_value());

    auto result{health_monitor_builder_add_deadline_monitor(
        health_monitor_builder_handle_.as_rust_handle().value(), &tag, monitor_handle.value())};
    abort_on_error(result);

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
    auto health_monitor_builder_handle = health_monitor_builder_handle_.drop_by_rust();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(health_monitor_builder_handle.has_value());

    uint32_t supervisor_duration_ms = static_cast<uint32_t>(supervisor_api_cycle_duration_.count());
    uint32_t internal_duration_ms = static_cast<uint32_t>(internal_processing_cycle_duration_.count());

    FFIHandle health_monitor_handle{nullptr};
    auto result{health_monitor_builder_build(
        health_monitor_builder_handle.value(), supervisor_duration_ms, internal_duration_ms, &health_monitor_handle)};
    abort_on_error(result);

    return HealthMonitor{health_monitor_handle};
}

HealthMonitor::HealthMonitor(FFIHandle handle) : health_monitor_(handle)
{
    // Initialize health monitor
}

HealthMonitor::HealthMonitor(HealthMonitor&& other)
{
    health_monitor_ = std::move(other.health_monitor_);
    other.health_monitor_ = nullptr;
}

score::cpp::expected<DeadlineMonitor, Error> HealthMonitor::get_deadline_monitor(const IdentTag& tag)
{
    FFIHandle handle{nullptr};
    auto result{health_monitor_get_deadline_monitor(health_monitor_, &tag, &handle)};
    if (result != kSuccess)
    {
        return score::cpp::unexpected(static_cast<Error>(result));
    }

    return score::cpp::expected<DeadlineMonitor, Error>(DeadlineMonitor{handle});
}

void HealthMonitor::start()
{
    auto result{health_monitor_start(health_monitor_)};
    abort_on_error(result);
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
