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
#include "score/health_monitor/mock_deadline.h"
#include "score/health_monitor/mock_deadline_monitor.h"
#include "score/health_monitor/mock_health_monitor.h"
#include "score/health_monitor/mock_heartbeat_monitor.h"
#include "score/health_monitor/mock_logic_monitor.h"
#include "score/health_monitor/mock_taken.h"

#include <gtest/gtest.h>

#include <optional>

using namespace score::health_monitor;
using score::health_monitor::testing::DeadlineMock;
using score::health_monitor::testing::DeadlineMonitorMock;
using score::health_monitor::testing::HealthMonitorMock;
using score::health_monitor::testing::HeartbeatMonitorMock;
using score::health_monitor::testing::LogicMonitorMock;
using score::health_monitor::testing::MakeTaken;

namespace
{

const MonitorTag kHeartbeatTag{"heartbeat"};
const MonitorTag kLogicTag{"logic"};
const MonitorTag kDeadlineMonitorTag{"deadlines"};
const DeadlineTag kDeadlineTag{"processing"};
const StateTag kRunningState{"running"};

class ExampleComponent
{
  public:
    score::Result<void> Init(HealthMonitor& health_monitor)
    {
        auto heartbeat{health_monitor.GetHeartbeatMonitor(kHeartbeatTag)};
        if (!heartbeat.has_value())
        {
            return score::MakeUnexpected<void>(heartbeat.error());
        }

        auto logic{health_monitor.GetLogicMonitor(kLogicTag)};
        if (!logic.has_value())
        {
            return score::MakeUnexpected<void>(logic.error());
        }

        const auto deadline_monitor{health_monitor.GetDeadlineMonitor(kDeadlineMonitorTag)};
        if (!deadline_monitor.has_value())
        {
            return score::MakeUnexpected<void>(deadline_monitor.error());
        }

        auto deadline{deadline_monitor.value()->GetDeadline(kDeadlineTag)};
        if (!deadline.has_value())
        {
            return score::MakeUnexpected<void>(deadline.error());
        }

        heartbeat_.emplace(std::move(heartbeat).value());
        logic_.emplace(std::move(logic).value());
        deadline_.emplace(std::move(deadline).value());
        return health_monitor.Start();
    }

    score::Result<void> Cycle()
    {
        (*heartbeat_)->Heartbeat();
        const auto transition{(*logic_)->Transition(kRunningState)};
        if (!transition.has_value())
        {
            return score::MakeUnexpected<void>(transition.error());
        }

        auto handle{(*deadline_)->Start()};
        if (!handle.has_value())
        {
            return score::MakeUnexpected<void>(handle.error());
        }

        return {};
    }

  private:
    std::optional<Taken<HeartbeatMonitor>> heartbeat_;
    std::optional<Taken<LogicMonitor>> logic_;
    std::optional<Taken<Deadline>> deadline_;
};

class MockUsageTest : public ::testing::Test
{
  protected:
    void ExpectMonitorsTaken()
    {
        EXPECT_CALL(health_monitor_, GetHeartbeatMonitor(kHeartbeatTag))
            .WillOnce(::testing::Return(MakeTaken(heartbeat_)));
        EXPECT_CALL(health_monitor_, GetLogicMonitor(kLogicTag)).WillOnce(::testing::Return(MakeTaken(logic_)));
        EXPECT_CALL(health_monitor_, GetDeadlineMonitor(kDeadlineMonitorTag))
            .WillOnce(::testing::Return(MakeTaken(deadline_monitor_)));
        EXPECT_CALL(deadline_monitor_, GetDeadline(kDeadlineTag)).WillOnce(::testing::Return(MakeTaken(deadline_)));
    }

    HealthMonitorMock health_monitor_;
    HeartbeatMonitorMock heartbeat_;
    LogicMonitorMock logic_;
    DeadlineMonitorMock deadline_monitor_;
    DeadlineMock deadline_;
    ExampleComponent component_;
};

class MockUsageCycleTest : public MockUsageTest
{
  protected:
    void SetUp() override
    {
        ExpectMonitorsTaken();
        EXPECT_CALL(health_monitor_, Start()).WillOnce(::testing::Return(score::Result<void>{}));
        ASSERT_TRUE(component_.Init(health_monitor_).has_value());
    }
};

}  // namespace

TEST_F(MockUsageTest, InitTakesMonitorsThenStartsHealthMonitor)
{
    ::testing::InSequence sequence;
    ExpectMonitorsTaken();
    EXPECT_CALL(health_monitor_, Start()).WillOnce(::testing::Return(score::Result<void>{}));

    EXPECT_TRUE(component_.Init(health_monitor_).has_value());
}

TEST_F(MockUsageTest, InitFailsOnUnknownTagAndNeverStarts)
{
    EXPECT_CALL(health_monitor_, GetHeartbeatMonitor(kHeartbeatTag))
        .WillOnce(::testing::Return(score::MakeUnexpected<Taken<HeartbeatMonitor>>(MakeError(ErrorCode::kUnknownTag))));
    EXPECT_CALL(health_monitor_, Start()).Times(0);

    const auto result{component_.Init(health_monitor_)};

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MakeError(ErrorCode::kUnknownTag));
}

TEST_F(MockUsageTest, InitFailsOnUnknownDeadlineTagAndNeverStarts)
{
    EXPECT_CALL(health_monitor_, GetHeartbeatMonitor(kHeartbeatTag)).WillOnce(::testing::Return(MakeTaken(heartbeat_)));
    EXPECT_CALL(health_monitor_, GetLogicMonitor(kLogicTag)).WillOnce(::testing::Return(MakeTaken(logic_)));
    EXPECT_CALL(health_monitor_, GetDeadlineMonitor(kDeadlineMonitorTag))
        .WillOnce(::testing::Return(MakeTaken(deadline_monitor_)));
    EXPECT_CALL(deadline_monitor_, GetDeadline(kDeadlineTag))
        .WillOnce(::testing::Return(score::MakeUnexpected<Taken<Deadline>>(MakeError(ErrorCode::kUnknownTag))));
    EXPECT_CALL(health_monitor_, Start()).Times(0);

    const auto result{component_.Init(health_monitor_)};

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MakeError(ErrorCode::kUnknownTag));
}

TEST_F(MockUsageTest, InitSurfacesStartError)
{
    ExpectMonitorsTaken();
    EXPECT_CALL(health_monitor_, Start())
        .WillOnce(::testing::Return(score::MakeUnexpected<void>(MakeError(ErrorCode::kNotTaken))));

    const auto result{component_.Init(health_monitor_)};

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MakeError(ErrorCode::kNotTaken));
}

TEST_F(MockUsageCycleTest, CycleHeartbeatsTransitionsAndRunsDeadlineInOrder)
{
    ::testing::InSequence sequence;
    EXPECT_CALL(heartbeat_, Heartbeat());
    EXPECT_CALL(logic_, Transition(kRunningState)).WillOnce(::testing::Return(kRunningState));
    EXPECT_CALL(deadline_, DoStart()).WillOnce(::testing::Return(score::Result<void>{}));
    EXPECT_CALL(deadline_, DoStop());

    EXPECT_TRUE(component_.Cycle().has_value());
}

TEST_F(MockUsageCycleTest, CycleSurfacesInvalidTransitionAndSkipsDeadline)
{
    EXPECT_CALL(heartbeat_, Heartbeat());
    EXPECT_CALL(logic_, Transition(kRunningState))
        .WillOnce(::testing::Return(score::MakeUnexpected<StateTag>(MakeError(ErrorCode::kInvalidTransition))));
    EXPECT_CALL(deadline_, DoStart()).Times(0);
    EXPECT_CALL(deadline_, DoStop()).Times(0);

    const auto result{component_.Cycle()};

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MakeError(ErrorCode::kInvalidTransition));
}

TEST_F(MockUsageCycleTest, CycleSurfacesDeadlineStartErrorWithoutStop)
{
    EXPECT_CALL(heartbeat_, Heartbeat());
    EXPECT_CALL(logic_, Transition(kRunningState)).WillOnce(::testing::Return(kRunningState));
    EXPECT_CALL(deadline_, DoStart())
        .WillOnce(::testing::Return(score::MakeUnexpected<void>(MakeError(ErrorCode::kDeadlineFailed))));
    EXPECT_CALL(deadline_, DoStop()).Times(0);

    const auto result{component_.Cycle()};

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), MakeError(ErrorCode::kDeadlineFailed));
}
