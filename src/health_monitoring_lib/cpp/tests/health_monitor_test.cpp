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
#include "score/hm/common.h"
#include "score/hm/tag.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>

using namespace score::hm;

class HealthMonitorTest : public ::testing::Test
{
};

// For first review round, only single test case to show up API
TEST_F(HealthMonitorTest, TestName)
{
    // Setup deadline monitor construction.
    const MonitorTag deadline_monitor_tag{"deadline_monitor"};
    auto deadline_monitor_builder =
        deadline::DeadlineMonitorBuilder()
            .add_deadline(DeadlineTag("deadline_1"),
                          TimeRange(std::chrono::milliseconds(100), std::chrono::milliseconds(200)))
            .add_deadline(DeadlineTag("deadline_2"),
                          TimeRange(std::chrono::milliseconds(100), std::chrono::milliseconds(200)));

    // Setup heartbeat monitor construction.
    const MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    const TimeRange heartbeat_range{std::chrono::milliseconds{100}, std::chrono::milliseconds{200}};
    auto heartbeat_monitor_builder = heartbeat::HeartbeatMonitorBuilder(heartbeat_range);

    auto hm = HealthMonitorBuilder()
                  .add_deadline_monitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                  .add_heartbeat_monitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                  .with_internal_processing_cycle(std::chrono::milliseconds(50))
                  .with_supervisor_api_cycle(std::chrono::milliseconds(50))
                  .build();

    // Obtain deadline monitor from HMON.
    auto deadline_monitor_res = hm.get_deadline_monitor(deadline_monitor_tag);
    EXPECT_TRUE(deadline_monitor_res.has_value());

    {
        // Try again to get the same monitor.
        auto deadline_monitor_res = hm.get_deadline_monitor(deadline_monitor_tag);
        EXPECT_FALSE(deadline_monitor_res.has_value());
    }

    auto deadline_mon = std::move(*deadline_monitor_res);

    // Obtain heartbeat monitor from HMON.
    auto heartbeat_monitor_res{hm.get_heartbeat_monitor(heartbeat_monitor_tag)};
    EXPECT_TRUE(heartbeat_monitor_res.has_value());

    {
        // Try again to get the same monitor.
        auto heartbeat_monitor_res{hm.get_heartbeat_monitor(heartbeat_monitor_tag)};
        EXPECT_FALSE(heartbeat_monitor_res.has_value());
    }

    auto heartbeat_monitor{std::move(*heartbeat_monitor_res)};

    // Start HMON.
    hm.start();

    auto deadline_res = deadline_mon.get_deadline(DeadlineTag("deadline_1"));

    {
        auto deadline_guard = deadline_res.value().start().value();

        EXPECT_EQ(deadline_res.value().start().error(), ::score::hm::Error::WrongState);
        deadline_guard.stop();
    }
}
