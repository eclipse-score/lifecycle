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

#include "score/mw/health/health_monitor.h"
#include "score/mw/health/deadline_monitor.h"
#include "score/mw/health/heartbeat_monitor.h"
#include "score/mw/health/logic_monitor.h"
#include <gtest/gtest.h>

using namespace score::mw::health;
using namespace score::mw::health::deadline;
using namespace score::mw::health::heartbeat;
using namespace score::mw::health::logic;

HeartbeatMonitorBuilder def_heartbeat_monitor_builder()
{
    using namespace std::chrono_literals;
    TimeRange range{100ms, 200ms};
    return HeartbeatMonitorBuilder{range};
}

LogicMonitorBuilder def_logic_monitor_builder()
{
    StateTag state1{"state1"};
    StateTag state2{"state2"};
    return LogicMonitorBuilder{state1}.AddState(state1, {state2}).AddState(state2, {state1});
}

class HealthMonitorBuilderFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(HealthMonitorBuilderFixture, New_Succeeds)
{
    RecordProperty("Description", "Object successfully constructed.");
    // Check able to construct and destruct only.
    HealthMonitorBuilder health_monitor_builder;
}

TEST_F(HealthMonitorBuilderFixture, Build_Succeeds)
{
    RecordProperty("Description", "Successfully build monitor containing all monitor types.");
    MonitorTag deadline_monitor_tag{"deadline_monitor"};
    DeadlineMonitorBuilder deadline_monitor_builder;
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    auto heartbeat_monitor_builder{def_heartbeat_monitor_builder()};
    MonitorTag logic_monitor_tag{"logic_monitor"};
    auto logic_monitor_builder{def_logic_monitor_builder()};

    auto result{HealthMonitorBuilder{}
                    .AddDeadlineMonitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                    .AddHeartbeatMonitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                    .AddLogicMonitor(logic_monitor_tag, std::move(logic_monitor_builder))
                    .Build()};
    ASSERT_TRUE(result.has_value());
}

TEST_F(HealthMonitorBuilderFixture, Build_InvalidCycles)
{
    RecordProperty(
        "Description",
        "Failed to build monitor with mismatched supervisor API cycle and internal processing cycle values.");
    using namespace std::chrono_literals;
    auto result{HealthMonitorBuilder{}.WithSupervisorApiCycle(123ms).WithInternalProcessingCycle(100ms).Build()};
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), Error::InvalidArgument);
}

TEST_F(HealthMonitorBuilderFixture, Build_NoMonitors)
{
    RecordProperty("Description", "Failed to build monitor with no monitors.");
    auto result{HealthMonitorBuilder{}.Build()};
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), Error::WrongState);
}

TEST(HealthMonitor, GetDeadlineMonitor_Available)
{
    RecordProperty("Description", "Successfully obtained deadline monitor.");
    MonitorTag deadline_monitor_tag{"deadline_monitor"};
    DeadlineMonitorBuilder deadline_monitor_builder;
    auto health_monitor{HealthMonitorBuilder{}
                            .AddDeadlineMonitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                            .Build()
                            .value()};

    auto result{health_monitor.GetDeadlineMonitor(deadline_monitor_tag)};
    ASSERT_TRUE(result.has_value());
}

TEST(HealthMonitor, GetDeadlineMonitor_Taken)
{
    RecordProperty("Description", "Failed to reobtain already taken deadline monitor.");
    MonitorTag deadline_monitor_tag{"deadline_monitor"};
    DeadlineMonitorBuilder deadline_monitor_builder;
    auto health_monitor{HealthMonitorBuilder{}
                            .AddDeadlineMonitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                            .Build()
                            .value()};

    health_monitor.GetDeadlineMonitor(deadline_monitor_tag);
    auto result{health_monitor.GetDeadlineMonitor(deadline_monitor_tag)};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetDeadlineMonitor_Unknown)
{
    RecordProperty("Description", "Failed to obtain deadline monitor using unknown tag.");
    MonitorTag deadline_monitor_tag{"deadline_monitor"};
    DeadlineMonitorBuilder deadline_monitor_builder;
    auto health_monitor{HealthMonitorBuilder{}
                            .AddDeadlineMonitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                            .Build()
                            .value()};

    auto result{health_monitor.GetDeadlineMonitor(MonitorTag{"undefined_monitor"})};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetHeartbeatMonitor_Available)
{
    RecordProperty("Description", "Successfully obtained heartbeat monitor.");
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    auto heartbeat_monitor_builder{def_heartbeat_monitor_builder()};
    auto health_monitor{HealthMonitorBuilder{}
                            .AddHeartbeatMonitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                            .Build()
                            .value()};

    auto result{health_monitor.GetHeartbeatMonitor(heartbeat_monitor_tag)};
    ASSERT_TRUE(result.has_value());
}

TEST(HealthMonitor, GetHeartbeatMonitor_Taken)
{
    RecordProperty("Description", "Failed to reobtain already taken heartbeat monitor.");
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    HeartbeatMonitorBuilder heartbeat_monitor_builder{def_heartbeat_monitor_builder()};
    auto health_monitor{HealthMonitorBuilder{}
                            .AddHeartbeatMonitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                            .Build()
                            .value()};

    health_monitor.GetHeartbeatMonitor(heartbeat_monitor_tag);
    auto result{health_monitor.GetHeartbeatMonitor(heartbeat_monitor_tag)};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetHeartbeatMonitor_Unknown)
{
    RecordProperty("Description", "Failed to obtain deadline monitor using unknown tag.");
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    HeartbeatMonitorBuilder heartbeat_monitor_builder{def_heartbeat_monitor_builder()};
    auto health_monitor{HealthMonitorBuilder{}
                            .AddHeartbeatMonitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                            .Build()
                            .value()};

    auto result{health_monitor.GetHeartbeatMonitor(MonitorTag{"undefined_monitor"})};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetLogicMonitor_Available)
{
    RecordProperty("Description", "Successfully obtained logic monitor.");
    MonitorTag logic_monitor_tag{"logic_monitor"};
    auto logic_monitor_builder{def_logic_monitor_builder()};
    auto health_monitor{
        HealthMonitorBuilder{}.AddLogicMonitor(logic_monitor_tag, std::move(logic_monitor_builder)).Build().value()};

    auto result{health_monitor.GetLogicMonitor(logic_monitor_tag)};
    ASSERT_TRUE(result.has_value());
}

TEST(HealthMonitor, GetLogicMonitor_Taken)
{
    RecordProperty("Description", "Failed to reobtain already taken logic monitor.");
    MonitorTag logic_monitor_tag{"logic_monitor"};
    LogicMonitorBuilder logic_monitor_builder{def_logic_monitor_builder()};
    auto health_monitor{
        HealthMonitorBuilder{}.AddLogicMonitor(logic_monitor_tag, std::move(logic_monitor_builder)).Build().value()};

    health_monitor.GetLogicMonitor(logic_monitor_tag);
    auto result{health_monitor.GetLogicMonitor(logic_monitor_tag)};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetLogicMonitor_Unknown)
{
    RecordProperty("Description", "Failed to obtain deadline monitor using unknown tag.");
    MonitorTag logic_monitor_tag{"logic_monitor"};
    LogicMonitorBuilder logic_monitor_builder{def_logic_monitor_builder()};
    auto health_monitor{
        HealthMonitorBuilder{}.AddLogicMonitor(logic_monitor_tag, std::move(logic_monitor_builder)).Build().value()};

    auto result{health_monitor.GetLogicMonitor(MonitorTag{"undefined_monitor"})};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, Start_Succeeds)
{
    RecordProperty("Description", "Successfully started monitor containing all monitor types.");
    MonitorTag deadline_monitor_tag{"deadline_monitor"};
    DeadlineMonitorBuilder deadline_monitor_builder;
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    auto heartbeat_monitor_builder{def_heartbeat_monitor_builder()};
    MonitorTag logic_monitor_tag{"logic_monitor"};
    auto logic_monitor_builder{def_logic_monitor_builder()};

    auto health_monitor{HealthMonitorBuilder{}
                            .AddDeadlineMonitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                            .AddHeartbeatMonitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                            .AddLogicMonitor(logic_monitor_tag, std::move(logic_monitor_builder))
                            .Build()
                            .value()};

    health_monitor.GetDeadlineMonitor(deadline_monitor_tag);
    health_monitor.GetHeartbeatMonitor(heartbeat_monitor_tag);
    health_monitor.GetLogicMonitor(logic_monitor_tag);

    health_monitor.Start();
}

TEST(HealthMonitor, Start_MonitorsNotTaken)
{
    RecordProperty("Description", "Failed to start of a health monitor with no monitors obtained.");
    MonitorTag deadline_monitor_tag{"deadline_monitor"};
    DeadlineMonitorBuilder deadline_monitor_builder;
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    auto heartbeat_monitor_builder{def_heartbeat_monitor_builder()};
    MonitorTag logic_monitor_tag{"logic_monitor"};
    auto logic_monitor_builder{def_logic_monitor_builder()};

    auto health_monitor{HealthMonitorBuilder{}
                            .AddDeadlineMonitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                            .AddHeartbeatMonitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                            .AddLogicMonitor(logic_monitor_tag, std::move(logic_monitor_builder))
                            .Build()
                            .value()};

    // `SIGABRT` is expected.
    ASSERT_DEATH({ health_monitor.Start(); }, "");
}
