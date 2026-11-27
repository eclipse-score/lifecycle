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

#include "score/health_monitor/src/cpp/supervisor_client/alive_supervisor_client.h"
#include "score/health_monitor/error.h"
#include "score/health_monitor/health_monitor.h"
#include "score/health_monitor/health_monitor_builder.h"
#include "score/health_monitor/heartbeat_monitor_builder.h"
#include "score/health_monitor/tag.h"
#include "score/health_monitor/time_range.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>

using namespace score::health_monitor;
using namespace score::launch_manager;
using namespace std::chrono_literals;

namespace
{

constexpr const char* kIdentifier{"IDENTIFIER"};
constexpr const char* kInterfacePath{"LCM_ALIVE_INTERFACE_PATH"};

std::optional<std::string> ReadEnv(const char* name)
{
    const char* value{std::getenv(name)};
    return (value == nullptr) ? std::nullopt : std::optional<std::string>{value};
}

void RestoreEnv(const char* name, const std::optional<std::string>& value)
{
    if (value.has_value())
    {
        ::setenv(name, value->c_str(), 1);
    }
    else
    {
        ::unsetenv(name);
    }
}

/// @brief Saves and restores the environment variables the client reads.
class AliveSupervisorClientTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        identifier_ = ReadEnv(kIdentifier);
        interface_path_ = ReadEnv(kInterfacePath);
    }

    void TearDown() override
    {
        RestoreEnv(kIdentifier, identifier_);
        RestoreEnv(kInterfacePath, interface_path_);
    }

    AliveSupervisorClient client_;

  private:
    std::optional<std::string> identifier_;
    std::optional<std::string> interface_path_;
};

}  // namespace

TEST_F(AliveSupervisorClientTest, ConnectFailsWithoutIdentifier)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that Connect returns kSupervisorUnavailable when IDENTIFIER is not set.");

    ::unsetenv(kIdentifier);

    const auto result = client_.Connect();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MakeError(ErrorCode::kSupervisorUnavailable));
}

TEST_F(AliveSupervisorClientTest, ConnectFailsWithoutInterfacePath)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Check that Connect returns kSupervisorUnavailable when IDENTIFIER is set but "
        "LCM_ALIVE_INTERFACE_PATH is not.");

    ::setenv(kIdentifier, "hmon_test", 1);
    ::unsetenv(kInterfacePath);

    const auto result = client_.Connect();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MakeError(ErrorCode::kSupervisorUnavailable));
}

TEST_F(AliveSupervisorClientTest, ConnectCanBeRepeatedAfterFailure)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description", "Check that Connect reports the failure again when it is called after a failed Connect.");

    ::unsetenv(kIdentifier);

    ASSERT_FALSE(client_.Connect().has_value());
    const auto second = client_.Connect();

    ASSERT_FALSE(second.has_value());
    EXPECT_EQ(second.error(), MakeError(ErrorCode::kSupervisorUnavailable));
}

TEST_F(AliveSupervisorClientTest, StartThroughThePublicApiFailsWithoutIdentifier)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Check that Start returns kSupervisorUnavailable when Build creates the alive client and "
        "IDENTIFIER is not set.");

    const MonitorTag heartbeat{"heartbeat"};
    auto built =
        HealthMonitorBuilder{}.AddHeartbeatMonitor(heartbeat, HeartbeatMonitorBuilder{TimeRange{100ms, 200ms}}).Build();
    ASSERT_TRUE(built.has_value());
    const auto health_monitor = std::move(built).value();
    auto taken = health_monitor->GetHeartbeatMonitor(heartbeat);
    ASSERT_TRUE(taken.has_value());
    ::unsetenv(kIdentifier);

    const auto result = health_monitor->Start();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MakeError(ErrorCode::kSupervisorUnavailable));
}
