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
#include "score/hm/heartbeat/heartbeat_monitor.h"

extern "C" {
using namespace score::hm;
using namespace score::hm::internal;
using namespace score::hm::heartbeat;

internal::FFIHandle heartbeat_monitor_builder_create(uint32_t range_min_ms, uint32_t range_max_ms);
void heartbeat_monitor_builder_destroy(internal::FFIHandle monitor_builder_handle);
void heartbeat_monitor_destroy(FFIHandle monitor_handle);
void heartbeat_monitor_heartbeat(FFIHandle monitor_handle);
}

namespace score::hm::heartbeat
{
HeartbeatMonitorBuilder::HeartbeatMonitorBuilder(const TimeRange& range)
    : monitor_builder_handle_{heartbeat_monitor_builder_create(range.min_ms(), range.max_ms()),
                              &heartbeat_monitor_builder_destroy}
{
}

HeartbeatMonitor::HeartbeatMonitor(FFIHandle monitor_handle)
    : monitor_handle_{monitor_handle, &heartbeat_monitor_destroy}
{
}

void HeartbeatMonitor::heartbeat()
{
    auto monitor_handle{monitor_handle_.as_rust_handle()};
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(monitor_handle.has_value());
    heartbeat_monitor_heartbeat(monitor_handle.value());
}

}  // namespace score::hm::heartbeat
