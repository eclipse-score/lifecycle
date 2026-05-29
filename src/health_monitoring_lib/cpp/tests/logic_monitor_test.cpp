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

#include "score/hm/logic/logic_monitor.h"
#include "score/hm/health_monitor.h"
#include <gtest/gtest.h>

using namespace score::hm;
using namespace score::hm::logic;

TEST(LogicMonitorBuilder, New_Succeeds)
{
    StateTag state1{"state1"};
    LogicMonitorBuilder logic_monitor_builder{state1};
}

TEST(LogicMonitorBuilder, AddState_Succeeds)
{
    StateTag state1{"state1"};
    StateTag state2{"state2"};
    auto logic_monitor_builder{LogicMonitorBuilder{state1}.add_state(state1, {state2})};
}

class LogicMonitorFixture : public ::testing::Test
{
  protected:
    std::optional<LogicMonitor> logic_monitor_;
    StateTag state1_{"state1"};
    StateTag state2_{"state2"};

    void SetUp() override
    {
        // Monitor must be obtained from HMON.
        // Initialize logic monitor builder.
        MonitorTag logic_monitor_tag{"logic_monitor"};
        auto logic_monitor_builder{LogicMonitorBuilder{state1_}.add_state(state1_, {state2_}).add_state(state2_, {})};

        // Build HMON, including logic monitor.
        auto hmon_build_result{
            HealthMonitorBuilder{}.add_logic_monitor(logic_monitor_tag, std::move(logic_monitor_builder)).build()};
        ASSERT_TRUE(hmon_build_result.has_value());
        auto hmon{std::move(hmon_build_result.value())};

        // Get logic monitor.
        auto get_logic_monitor_result{hmon.get_logic_monitor(logic_monitor_tag)};
        ASSERT_TRUE(get_logic_monitor_result.has_value());
        logic_monitor_ = std::move(get_logic_monitor_result.value());
    }
};

TEST_F(LogicMonitorFixture, Transition_Succeeds)
{  // State transition.
    auto transition_result{logic_monitor_->transition(state2_)};
    ASSERT_TRUE(transition_result.has_value());
    ASSERT_EQ(transition_result.value(), state2_);
}

TEST_F(LogicMonitorFixture, Transition_Unknown)
{
    // State transition.
    auto transition_result{logic_monitor_->transition(StateTag{"unknown"})};
    ASSERT_FALSE(transition_result.has_value());
    ASSERT_EQ(transition_result.error(), Error::Failed);
}

TEST_F(LogicMonitorFixture, Transition_Invalid)
{
    // State transition into invalid state.
    logic_monitor_->transition(StateTag{"unknown"});

    // State transition.
    auto transition_result{logic_monitor_->transition(state2_)};
    ASSERT_FALSE(transition_result.has_value());
    ASSERT_EQ(transition_result.error(), Error::Failed);
}

TEST_F(LogicMonitorFixture, State_Succeeds)
{
    // State transition.
    logic_monitor_->transition(state2_);

    // Get state.
    auto state_result{logic_monitor_->state()};
    ASSERT_TRUE(state_result.has_value());
    ASSERT_EQ(state_result.value(), state2_);
}

TEST_F(LogicMonitorFixture, State_Invalid)
{
    // State transition.
    logic_monitor_->transition(StateTag{"unknown"});

    // Get state.
    auto state_result{logic_monitor_->state()};
    ASSERT_FALSE(state_result.has_value());
    ASSERT_EQ(state_result.error(), Error::Failed);
}
