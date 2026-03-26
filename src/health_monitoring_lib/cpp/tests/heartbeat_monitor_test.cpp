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
#include "score/hm/health_monitor.h"
#include <gtest/gtest.h>

using namespace score::hm;
using namespace score::hm::heartbeat;

TEST(HeartbeatMonitorBuilder, New_Succeeds)
{
    using namespace std::chrono_literals;
    TimeRange range{100ms, 200ms};
    HeartbeatMonitorBuilder heartbeat_monitor_builder{range};
}

TEST(HeartbeatMonitor, Heartbeat_Succeeds)
{
    // Monitor must be obtained from HMON.
    // Initialize heartbeat monitor builder.
    using namespace std::chrono_literals;
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    TimeRange range{100ms, 200ms};
    HeartbeatMonitorBuilder heartbeat_monitor_builder{range};

    // Build HMON, including heartbeat monitor.
    auto hmon_build_result{HealthMonitorBuilder{}
                               .add_heartbeat_monitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                               .build()};
    ASSERT_TRUE(hmon_build_result.has_value());
    auto hmon{std::move(hmon_build_result.value())};

    // Get heartbeat monitor.
    auto get_heartbeat_monitor_result{hmon.get_heartbeat_monitor(heartbeat_monitor_tag)};
    ASSERT_TRUE(get_heartbeat_monitor_result.has_value());
    auto heartbeat_monitor{std::move(get_heartbeat_monitor_result.value())};

    // Check heartbeat is not failing.
    heartbeat_monitor.heartbeat();
}
