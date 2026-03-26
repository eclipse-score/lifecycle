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
#include "score/hm/deadline/deadline_monitor.h"
#include "score/hm/heartbeat/heartbeat_monitor.h"
#include "score/hm/logic/logic_monitor.h"
#include <gtest/gtest.h>

using namespace score::hm;
using namespace score::hm::deadline;
using namespace score::hm::heartbeat;
using namespace score::hm::logic;

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
    return LogicMonitorBuilder{state1}.add_state(state1, {state2}).add_state(state2, {state1});
}

TEST(HealthMonitorBuilder, New_Succeeds)
{
    // Check able to construct and destruct only.
    HealthMonitorBuilder health_monitor_builder;
}

TEST(HealthMonitorBuilder, Build_Succeeds)
{
    MonitorTag deadline_monitor_tag{"deadline_monitor"};
    DeadlineMonitorBuilder deadline_monitor_builder;
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    auto heartbeat_monitor_builder{def_heartbeat_monitor_builder()};
    MonitorTag logic_monitor_tag{"logic_monitor"};
    auto logic_monitor_builder{def_logic_monitor_builder()};

    auto result{HealthMonitorBuilder{}
                    .add_deadline_monitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                    .add_heartbeat_monitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                    .add_logic_monitor(logic_monitor_tag, std::move(logic_monitor_builder))
                    .build()};
    ASSERT_TRUE(result.has_value());
}

TEST(HealthMonitorBuilder, Build_InvalidCycles)
{
    using namespace std::chrono_literals;
    auto result{HealthMonitorBuilder{}.with_supervisor_api_cycle(123ms).with_internal_processing_cycle(100ms).build()};
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), Error::InvalidArgument);
}

TEST(HealthMonitorBuilder, Build_NoMonitors)
{
    auto result{HealthMonitorBuilder{}.build()};
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), Error::WrongState);
}

TEST(HealthMonitor, GetDeadlineMonitor_Available)
{
    MonitorTag deadline_monitor_tag{"deadline_monitor"};
    DeadlineMonitorBuilder deadline_monitor_builder;
    auto health_monitor{HealthMonitorBuilder{}
                            .add_deadline_monitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                            .build()
                            .value()};

    auto result{health_monitor.get_deadline_monitor(deadline_monitor_tag)};
    ASSERT_TRUE(result.has_value());
}

TEST(HealthMonitor, GetDeadlineMonitor_Taken)
{
    MonitorTag deadline_monitor_tag{"deadline_monitor"};
    DeadlineMonitorBuilder deadline_monitor_builder;
    auto health_monitor{HealthMonitorBuilder{}
                            .add_deadline_monitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                            .build()
                            .value()};

    health_monitor.get_deadline_monitor(deadline_monitor_tag);
    auto result{health_monitor.get_deadline_monitor(deadline_monitor_tag)};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetDeadlineMonitor_Unknown)
{
    MonitorTag deadline_monitor_tag{"deadline_monitor"};
    DeadlineMonitorBuilder deadline_monitor_builder;
    auto health_monitor{HealthMonitorBuilder{}
                            .add_deadline_monitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                            .build()
                            .value()};

    auto result{health_monitor.get_deadline_monitor(MonitorTag{"undefined_monitor"})};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetHeartbeatMonitor_Available)
{
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    auto heartbeat_monitor_builder{def_heartbeat_monitor_builder()};
    auto health_monitor{HealthMonitorBuilder{}
                            .add_heartbeat_monitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                            .build()
                            .value()};

    auto result{health_monitor.get_heartbeat_monitor(heartbeat_monitor_tag)};
    ASSERT_TRUE(result.has_value());
}

TEST(HealthMonitor, GetHeartbeatMonitor_Taken)
{
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    HeartbeatMonitorBuilder heartbeat_monitor_builder{def_heartbeat_monitor_builder()};
    auto health_monitor{HealthMonitorBuilder{}
                            .add_heartbeat_monitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                            .build()
                            .value()};

    health_monitor.get_heartbeat_monitor(heartbeat_monitor_tag);
    auto result{health_monitor.get_heartbeat_monitor(heartbeat_monitor_tag)};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetHeartbeatMonitor_Unknown)
{
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    HeartbeatMonitorBuilder heartbeat_monitor_builder{def_heartbeat_monitor_builder()};
    auto health_monitor{HealthMonitorBuilder{}
                            .add_heartbeat_monitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                            .build()
                            .value()};

    auto result{health_monitor.get_heartbeat_monitor(MonitorTag{"undefined_monitor"})};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetLogicMonitor_Available)
{
    MonitorTag logic_monitor_tag{"logic_monitor"};
    auto logic_monitor_builder{def_logic_monitor_builder()};
    auto health_monitor{
        HealthMonitorBuilder{}.add_logic_monitor(logic_monitor_tag, std::move(logic_monitor_builder)).build().value()};

    auto result{health_monitor.get_logic_monitor(logic_monitor_tag)};
    ASSERT_TRUE(result.has_value());
}

TEST(HealthMonitor, GetLogicMonitor_Taken)
{
    MonitorTag logic_monitor_tag{"logic_monitor"};
    LogicMonitorBuilder logic_monitor_builder{def_logic_monitor_builder()};
    auto health_monitor{
        HealthMonitorBuilder{}.add_logic_monitor(logic_monitor_tag, std::move(logic_monitor_builder)).build().value()};

    health_monitor.get_logic_monitor(logic_monitor_tag);
    auto result{health_monitor.get_logic_monitor(logic_monitor_tag)};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, GetLogicMonitor_Unknown)
{
    MonitorTag logic_monitor_tag{"logic_monitor"};
    LogicMonitorBuilder logic_monitor_builder{def_logic_monitor_builder()};
    auto health_monitor{
        HealthMonitorBuilder{}.add_logic_monitor(logic_monitor_tag, std::move(logic_monitor_builder)).build().value()};

    auto result{health_monitor.get_logic_monitor(MonitorTag{"undefined_monitor"})};
    ASSERT_FALSE(result.has_value());
}

TEST(HealthMonitor, Start_Succeeds)
{
    MonitorTag deadline_monitor_tag{"deadline_monitor"};
    DeadlineMonitorBuilder deadline_monitor_builder;
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    auto heartbeat_monitor_builder{def_heartbeat_monitor_builder()};
    MonitorTag logic_monitor_tag{"logic_monitor"};
    auto logic_monitor_builder{def_logic_monitor_builder()};

    auto health_monitor{HealthMonitorBuilder{}
                            .add_deadline_monitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                            .add_heartbeat_monitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                            .add_logic_monitor(logic_monitor_tag, std::move(logic_monitor_builder))
                            .build()
                            .value()};

    health_monitor.get_deadline_monitor(deadline_monitor_tag);
    health_monitor.get_heartbeat_monitor(heartbeat_monitor_tag);
    health_monitor.get_logic_monitor(logic_monitor_tag);

    health_monitor.start();
}

TEST(HealthMonitor, Start_MonitorsNotTaken)
{
    MonitorTag deadline_monitor_tag{"deadline_monitor"};
    DeadlineMonitorBuilder deadline_monitor_builder;
    MonitorTag heartbeat_monitor_tag{"heartbeat_monitor"};
    auto heartbeat_monitor_builder{def_heartbeat_monitor_builder()};
    MonitorTag logic_monitor_tag{"logic_monitor"};
    auto logic_monitor_builder{def_logic_monitor_builder()};

    auto health_monitor{HealthMonitorBuilder{}
                            .add_deadline_monitor(deadline_monitor_tag, std::move(deadline_monitor_builder))
                            .add_heartbeat_monitor(heartbeat_monitor_tag, std::move(heartbeat_monitor_builder))
                            .add_logic_monitor(logic_monitor_tag, std::move(logic_monitor_builder))
                            .build()
                            .value()};

    // `SIGABRT` is expected.
    ASSERT_DEATH({ health_monitor.start(); }, "");
}
