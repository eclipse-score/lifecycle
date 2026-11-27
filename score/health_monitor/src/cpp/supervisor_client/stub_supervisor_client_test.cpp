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

#include "score/health_monitor/error.h"
#include "score/health_monitor/health_monitor.h"
#include "score/health_monitor/health_monitor_builder.h"
#include "score/health_monitor/heartbeat_monitor.h"
#include "score/health_monitor/heartbeat_monitor_builder.h"
#include "score/health_monitor/src/cpp/internal/test_helpers.h"
#include "score/health_monitor/tag.h"
#include "score/health_monitor/taken.h"
#include "score/health_monitor/time_range.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <optional>
#include <utility>

using namespace score::health_monitor;
using namespace score::health_monitor::details;
using namespace std::chrono_literals;

namespace
{

const MonitorTag kHeartbeat{"heartbeat"};

/// @brief Builds a health monitor with one heartbeat monitor and takes that monitor.
class StubSupervisorClientTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        auto built = HealthMonitorBuilder{}
                         .AddHeartbeatMonitor(kHeartbeat, HeartbeatMonitorBuilder{TimeRange{100ms, 200ms}})
                         .Build();
        ASSERT_TRUE(built.has_value());
        health_monitor_ = std::move(built).value();
        auto taken = health_monitor_->GetHeartbeatMonitor(kHeartbeat);
        ASSERT_TRUE(taken.has_value());
        heartbeat_monitor_.emplace(std::move(taken).value());
    }

    std::unique_ptr<HealthMonitor> health_monitor_;
    std::optional<Taken<HeartbeatMonitor>> heartbeat_monitor_;
};

}  // namespace

TEST_F(StubSupervisorClientTest, StartSucceedsThroughThePublicApiWithoutLaunchManager)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description",
        "Check that Build, GetHeartbeatMonitor and Start succeed through the public API with the stub client.");

    EXPECT_TRUE(health_monitor_->Start().has_value());
}

TEST_F(StubSupervisorClientTest, SecondStartFailsWithAlreadyStarted)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that a second Start through the public API fails with kAlreadyStarted.");

    ASSERT_TRUE(health_monitor_->Start().has_value());

    ExpectError(health_monitor_->Start(), ErrorCode::kAlreadyStarted);
}
