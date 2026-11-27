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
#include <gtest/gtest.h>
#include <thread>

#include "score/health_monitor/health_monitor.h"
#include "score/health_monitor/health_monitor_builder.h"
#include "score/health_monitor/heartbeat_monitor_builder.h"
#include "score/health_monitor/tag.h"
#include "score/health_monitor/time_range.h"
#include "score/mw/lifecycle/report_running.h"
#include "tests/utils/test_helper/test_runner.hpp"

TEST(ComplexMonitoring, ComponentComplexMonitoring)
{
    using namespace score::health_monitor;
    using namespace std::chrono_literals;

    constexpr MonitorTag kMonitorTag{"complex_monitoring_monitor"};

    auto hm_result = HealthMonitorBuilder{}
                         .AddHeartbeatMonitor(kMonitorTag, HeartbeatMonitorBuilder{TimeRange{50ms, 150ms}})
                         .WithInternalProcessingCycle(50ms)
                         .WithSupervisorApiCycle(50ms)
                         .Build();
    ASSERT_TRUE(hm_result.has_value()) << "Failed to build HealthMonitor";
    const auto hm = std::move(hm_result).value();

    auto heartbeat_monitor_result = hm->GetHeartbeatMonitor(kMonitorTag);
    ASSERT_TRUE(heartbeat_monitor_result.has_value()) << "Failed to get heartbeat monitor";
    const auto heartbeat_monitor = std::move(heartbeat_monitor_result).value();

    ASSERT_TRUE(hm->Start().has_value()) << "Failed to start HealthMonitor";

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    auto time_to_report_checkpoints_until = std::chrono::steady_clock::now() + 1s;
    // Given that we can send heartbeats successfully...
    TEST_STEP("Send heartbeats for 1 second")
    {
        while (std::chrono::steady_clock::now() < time_to_report_checkpoints_until)
        {
            std::this_thread::sleep_for(75ms);
            heartbeat_monitor->Heartbeat();
        }
        EXPECT_FALSE(TestRunner::g_exit_requested) << "Process should not be terminated yet";
    }
    // When heartbeats are no longer sent...
}

int main()
{
    TestRunner::Init();
    TestRunner::RunTests();
    // Then expect kill due to recovery action (verified by state manager)
    while (true)  // Stop reporting, wait for sigkill
    {
        pause();
    }
}
