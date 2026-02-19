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
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

using namespace score::hm;
using ::testing::_;

class HealthMonitorTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");
    }
};

// For first review round, only single test case to show up API
TEST_F(HealthMonitorTest, TestName)
{
    RecordProperty(
        "Description",
        "This test demonstrates the usage of HealthMonitor and DeadlineMonitor APIs. It creates a HealthMonitor with a "
        "DeadlineMonitor, retrieves the DeadlineMonitor, and tests starting a deadline.");
    auto builder_mon = deadline::DeadlineMonitorBuilder()
                           .add_deadline(IdentTag("deadline_1"),
                                         TimeRange(std::chrono::milliseconds(100), std::chrono::milliseconds(200)))
                           .add_deadline(IdentTag("deadline_2"),
                                         TimeRange(std::chrono::milliseconds(100), std::chrono::milliseconds(200)));

    IdentTag ident("monitor");

    auto hm = HealthMonitorBuilder()
                  .add_deadline_monitor(ident, std::move(builder_mon))
                  .with_internal_processing_cycle(std::chrono::milliseconds(50))
                  .with_supervisor_api_cycle(std::chrono::milliseconds(50))
                  .build();

    auto deadline_monitor_res = hm.get_deadline_monitor(ident);
    EXPECT_TRUE(deadline_monitor_res.has_value());

    {
        // Try again to get the same monitor
        auto deadline_monitor_res = hm.get_deadline_monitor(ident);
        EXPECT_FALSE(deadline_monitor_res.has_value());
    }

    auto deadline_mon = std::move(*deadline_monitor_res);

    hm.start();

    auto deadline_res = deadline_mon.get_deadline(IdentTag("deadline_1"));

    {
        auto deadline_guard = deadline_res.value().start().value();

        EXPECT_EQ(deadline_res.value().start().error(), ::score::hm::Error::WrongState);
        deadline_guard.stop();
    }
}
