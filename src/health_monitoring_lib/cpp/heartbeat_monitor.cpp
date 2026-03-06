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

namespace {
extern "C" {
using namespace score::hm;
using namespace score::hm::internal;
using namespace score::hm::heartbeat;

FFICode heartbeat_monitor_builder_create(uint32_t range_min_ms, uint32_t range_max_ms, FFIHandle* heartbeat_monitor_builder_handle_out);
FFICode heartbeat_monitor_builder_destroy(FFIHandle heartbeat_monitor_builder_handle);
FFICode heartbeat_monitor_destroy(FFIHandle heartbeat_monitor_builder_handle);
FFICode heartbeat_monitor_heartbeat(FFIHandle heartbeat_monitor_builder_handle);
}

FFIHandle heartbeat_monitor_builder_create_wrapper(uint32_t range_min_ms, uint32_t range_max_ms)
{
    FFIHandle handle{nullptr};
    auto result{heartbeat_monitor_builder_create(range_min_ms, range_max_ms, &handle)};
    SCORE_LANGUAGE_FUTURECPP_ASSERT(result == kSuccess);
    return handle;
}

}

namespace score::hm::heartbeat
{
HeartbeatMonitorBuilder::HeartbeatMonitorBuilder(const TimeRange& range)
    : monitor_builder_handle_{heartbeat_monitor_builder_create_wrapper(range.min_ms(), range.max_ms()),
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
    SCORE_LANGUAGE_FUTURECPP_ASSERT(heartbeat_monitor_heartbeat(monitor_handle.value()) == kSuccess);
}

}  // namespace score::hm::heartbeat
