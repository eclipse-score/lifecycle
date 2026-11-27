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

#include "score/health_monitor/src/cpp/internal/health_monitor_impl.h"

#include "score/health_monitor/error.h"
#include "score/health_monitor/src/cpp/internal/mock_supervisor_client.h"
#include "score/health_monitor/src/cpp/internal/test_helpers.h"
#include "score/os/time.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <utility>

using namespace score::health_monitor;
using namespace score::health_monitor::details;
using namespace std::chrono_literals;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::StrictMock;

namespace
{

const MonitorTag kHeartbeat{"heartbeat"};
const MonitorTag kSecondHeartbeat{"second_heartbeat"};
const MonitorTag kLogic{"logic"};
const MonitorTag kUnknown{"unknown"};
const StateTag kStateA{"a"};
const StateTag kStateB{"b"};
const StateTag kStateOther{"other"};
score::Result<void> Success()
{
    return {};
}

/// @brief Health monitor with heartbeat monitors "heartbeat" and "second_heartbeat" over a strict client mock.
/// Cycles are short because the worker runs on real time.
class HealthMonitorImplTest : public ::testing::Test
{
  protected:
    static constexpr std::chrono::milliseconds kInternalCycle{10};
    /// @brief Heartbeat range wide enough that an unattended monitor stays healthy for the whole test.
    static constexpr std::chrono::milliseconds kHealthyMin{1000};
    static constexpr std::chrono::milliseconds kHealthyMax{5000};
    /// @brief Heartbeat range that a monitor without heartbeats leaves within about 50 ms.
    static constexpr std::chrono::milliseconds kShortMin{20};
    static constexpr std::chrono::milliseconds kShortMax{40};

    void Create(
        const std::chrono::milliseconds range_min,
        const std::chrono::milliseconds range_max,
        const bool second_monitor = false)
    {
        HealthMonitorImpl::Config config;
        config.heartbeat_monitors.push_back(
            HealthMonitorImpl::Config::HeartbeatMonitorConfig{kHeartbeat, TimeRange{range_min, range_max}});
        if (second_monitor)
        {
            config.heartbeat_monitors.push_back(
                HealthMonitorImpl::Config::HeartbeatMonitorConfig{kSecondHeartbeat, TimeRange{range_min, range_max}});
        }
        config.internal_processing_cycle = kInternalCycle;
        config.supervisor_api_cycle = 10ms;

        auto client = std::make_unique<StrictMock<SupervisorClientMock>>();
        client_ = client.get();
        auto result = HealthMonitorImpl::Create(std::move(config), std::move(client), score::os::Time::instance());
        ASSERT_TRUE(result.has_value());
        monitor_ = std::move(result).value();
    }

    void CreateWithLogic()
    {
        HealthMonitorImpl::Config config;
        config.heartbeat_monitors.push_back(
            HealthMonitorImpl::Config::HeartbeatMonitorConfig{kHeartbeat, TimeRange{kHealthyMin, kHealthyMax}});
        config.logic_monitors.push_back(
            HealthMonitorImpl::Config::LogicMonitorConfig{
                kLogic, kStateA, {StateNode{kStateA, {kStateB}}, StateNode{kStateB, {kStateA}}}});
        config.internal_processing_cycle = kInternalCycle;
        config.supervisor_api_cycle = 10ms;

        auto client = std::make_unique<StrictMock<SupervisorClientMock>>();
        client_ = client.get();
        auto result = HealthMonitorImpl::Create(std::move(config), std::move(client), score::os::Time::instance());
        ASSERT_TRUE(result.has_value());
        monitor_ = std::move(result).value();
    }

    void ExpectConnectAndNotify()
    {
        EXPECT_CALL(*client_, Connect()).WillOnce(Invoke(&Success));
        EXPECT_CALL(*client_, NotifyAlive()).Times(AnyNumber());
    }

    /// @brief Counts `NotifyAlive` calls, declared before `monitor_` so the worker never outlives it.
    std::atomic<int> notifications_{0};
    StrictMock<SupervisorClientMock>* client_{nullptr};
    std::unique_ptr<HealthMonitorImpl> monitor_;
};

}  // namespace

TEST_F(HealthMonitorImplTest, HeartbeatMonitorCanBeTakenOnce)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that GetHeartbeatMonitor returns a configured monitor.");

    Create(kHealthyMin, kHealthyMax);

    auto result = monitor_->GetHeartbeatMonitor(kHeartbeat);

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(&result.value().Get(), nullptr);
}

TEST_F(HealthMonitorImplTest, SecondTakeOfTheSameMonitorFailsWithAlreadyTaken)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that taking a heartbeat monitor twice fails with kAlreadyTaken.");

    Create(kHealthyMin, kHealthyMax);
    auto first = monitor_->GetHeartbeatMonitor(kHeartbeat);
    ASSERT_TRUE(first.has_value());

    ExpectError(monitor_->GetHeartbeatMonitor(kHeartbeat), ErrorCode::kAlreadyTaken);
}

TEST_F(HealthMonitorImplTest, TakingOneMonitorDoesNotTakeTheOther)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that taking one heartbeat monitor leaves the other one available.");

    Create(kHealthyMin, kHealthyMax, true);
    auto first = monitor_->GetHeartbeatMonitor(kHeartbeat);
    ASSERT_TRUE(first.has_value());

    auto second = monitor_->GetHeartbeatMonitor(kSecondHeartbeat);

    ASSERT_TRUE(second.has_value());
    EXPECT_NE(&first.value().Get(), &second.value().Get());
}

TEST_F(HealthMonitorImplTest, UnknownHeartbeatTagFailsWithUnknownTag)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that GetHeartbeatMonitor with an unconfigured tag fails with kUnknownTag.");

    Create(kHealthyMin, kHealthyMax);

    ExpectError(monitor_->GetHeartbeatMonitor(kUnknown), ErrorCode::kUnknownTag);
}

TEST_F(HealthMonitorImplTest, DeadlineGetterFailsWithUnknownTag)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that GetDeadlineMonitor fails with kUnknownTag, also for a heartbeat tag.");

    Create(kHealthyMin, kHealthyMax);

    ExpectError(monitor_->GetDeadlineMonitor(kUnknown), ErrorCode::kUnknownTag);
    ExpectError(monitor_->GetDeadlineMonitor(kHeartbeat), ErrorCode::kUnknownTag);
}

TEST_F(HealthMonitorImplTest, LogicMonitorCanBeTakenOnce)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that GetLogicMonitor returns a configured monitor in its initial state.");

    CreateWithLogic();

    auto result = monitor_->GetLogicMonitor(kLogic);

    ASSERT_TRUE(result.has_value());
    const auto state = result.value()->State();
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state.value(), kStateA);
}

TEST_F(HealthMonitorImplTest, SecondTakeOfTheLogicMonitorFailsWithAlreadyTaken)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that taking a logic monitor twice fails with kAlreadyTaken.");

    CreateWithLogic();
    auto first = monitor_->GetLogicMonitor(kLogic);
    ASSERT_TRUE(first.has_value());

    ExpectError(monitor_->GetLogicMonitor(kLogic), ErrorCode::kAlreadyTaken);
}

TEST_F(HealthMonitorImplTest, LogicGetterFailsWithUnknownTagForOtherTags)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Check that GetLogicMonitor fails with kUnknownTag for an unconfigured tag and a heartbeat tag, "
        "and GetHeartbeatMonitor for a logic tag.");

    CreateWithLogic();

    ExpectError(monitor_->GetLogicMonitor(kUnknown), ErrorCode::kUnknownTag);
    ExpectError(monitor_->GetLogicMonitor(kHeartbeat), ErrorCode::kUnknownTag);
    ExpectError(monitor_->GetHeartbeatMonitor(kLogic), ErrorCode::kUnknownTag);
}

TEST_F(HealthMonitorImplTest, StartFailsWithNotTakenWhileTheLogicMonitorIsUntaken)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description",
        "Check that Start fails with kNotTaken without connecting while only the logic monitor is "
        "untaken.");

    CreateWithLogic();
    auto heartbeat = monitor_->GetHeartbeatMonitor(kHeartbeat);
    ASSERT_TRUE(heartbeat.has_value());

    // The strict mock fails the test if Start connects.
    ExpectError(monitor_->Start(), ErrorCode::kNotTaken);
}

TEST_F(HealthMonitorImplTest, InvalidLogicTransitionStopsTheAliveNotifications)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__alive_withhold, comp_req__health_monitor__lg_invalid_trans");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description",
        "Check that the worker notifies alive while the logic monitor is healthy and stops after an "
        "invalid transition.");

    CreateWithLogic();
    EXPECT_CALL(*client_, Connect()).WillOnce(Invoke(&Success));
    EXPECT_CALL(*client_, NotifyAlive()).Times(AnyNumber()).WillRepeatedly(Invoke([this] {
        ++notifications_;
    }));
    auto heartbeat = monitor_->GetHeartbeatMonitor(kHeartbeat);
    ASSERT_TRUE(heartbeat.has_value());
    auto logic = monitor_->GetLogicMonitor(kLogic);
    ASSERT_TRUE(logic.has_value());
    ASSERT_TRUE(monitor_->Start().has_value());
    ASSERT_TRUE(WaitFor([&] {
        return notifications_.load() >= 1;
    }));

    ExpectError(logic.value()->Transition(kStateOther), ErrorCode::kInvalidTransition);

    // The worker counts as stopped once the notifications stay unchanged for five internal cycles.
    int last_count{-1};
    auto last_change = std::chrono::steady_clock::now();
    EXPECT_TRUE(WaitFor(
        [&] {
            const int count{notifications_.load()};
            const auto now = std::chrono::steady_clock::now();
            if (count != last_count)
            {
                last_count = count;
                last_change = now;
            }
            return (now - last_change >= 5 * kInternalCycle);
        },
        5s));
}

TEST_F(HealthMonitorImplTest, StartFailsWithNotTakenWhileAMonitorIsUntaken)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description", "Check that Start fails with kNotTaken without connecting while one monitor is untaken.");

    Create(kHealthyMin, kHealthyMax, true);
    auto first = monitor_->GetHeartbeatMonitor(kHeartbeat);
    ASSERT_TRUE(first.has_value());

    // The strict mock fails the test if Start connects.
    ExpectError(monitor_->Start(), ErrorCode::kNotTaken);
}

TEST_F(HealthMonitorImplTest, StartSucceedsOnceEveryMonitorIsTakenAndNotifiesAlive)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__monitor_once, comp_req__health_monitor__alive_notify");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that Start connects once and the worker then notifies alive.");

    Create(kHealthyMin, kHealthyMax);
    EXPECT_CALL(*client_, Connect()).WillOnce(Invoke(&Success));
    EXPECT_CALL(*client_, NotifyAlive()).Times(AnyNumber()).WillRepeatedly(Invoke([this] {
        ++notifications_;
    }));
    auto taken = monitor_->GetHeartbeatMonitor(kHeartbeat);
    ASSERT_TRUE(taken.has_value());

    ASSERT_TRUE(monitor_->Start().has_value());

    EXPECT_TRUE(WaitFor([&] {
        return notifications_.load() >= 1;
    }));
}

TEST_F(HealthMonitorImplTest, SecondStartFailsWithAlreadyStarted)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that Start after a successful Start fails with kAlreadyStarted.");

    Create(kHealthyMin, kHealthyMax);
    ExpectConnectAndNotify();
    auto taken = monitor_->GetHeartbeatMonitor(kHeartbeat);
    ASSERT_TRUE(taken.has_value());
    ASSERT_TRUE(monitor_->Start().has_value());

    ExpectError(monitor_->Start(), ErrorCode::kAlreadyStarted);
}

TEST_F(HealthMonitorImplTest, GettersFailWithAlreadyStartedAfterStart)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that every getter fails with kAlreadyStarted after Start, whatever the tag.");

    Create(kHealthyMin, kHealthyMax);
    ExpectConnectAndNotify();
    auto taken = monitor_->GetHeartbeatMonitor(kHeartbeat);
    ASSERT_TRUE(taken.has_value());
    ASSERT_TRUE(monitor_->Start().has_value());

    ExpectError(monitor_->GetHeartbeatMonitor(kHeartbeat), ErrorCode::kAlreadyStarted);
    ExpectError(monitor_->GetHeartbeatMonitor(kUnknown), ErrorCode::kAlreadyStarted);
    ExpectError(monitor_->GetDeadlineMonitor(kUnknown), ErrorCode::kAlreadyStarted);
    ExpectError(monitor_->GetLogicMonitor(kUnknown), ErrorCode::kAlreadyStarted);
}

TEST_F(HealthMonitorImplTest, GettersAndStartFailWithAlreadyStartedAfterTheWorkerStoppedOnAFailure)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_once");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description",
        "Check that a health monitor whose worker stopped on a failure still reports kAlreadyStarted and "
        "does not restart.");

    Create(kShortMin, kShortMax);
    EXPECT_CALL(*client_, Connect()).WillOnce(Invoke(&Success));
    EXPECT_CALL(*client_, NotifyAlive()).Times(AnyNumber()).WillRepeatedly(Invoke([this] {
        ++notifications_;
    }));
    auto taken = monitor_->GetHeartbeatMonitor(kHeartbeat);
    ASSERT_TRUE(taken.has_value());
    ASSERT_TRUE(monitor_->Start().has_value());

    // The monitor never beats, so the worker fails once the range maximum passes.
    // The worker counts as stopped once the notifications stay unchanged for five internal cycles.
    int last_count{-1};
    auto last_change = std::chrono::steady_clock::now();
    const auto stopped = WaitFor([&] {
        const int count{notifications_.load()};
        const auto now = std::chrono::steady_clock::now();
        if (count != last_count)
        {
            last_count = count;
            last_change = now;
        }
        return (count > 0) && (now - last_change >= 5 * kInternalCycle);
    });
    ASSERT_TRUE(stopped);

    ExpectError(monitor_->GetHeartbeatMonitor(kHeartbeat), ErrorCode::kAlreadyStarted);
    ExpectError(monitor_->GetDeadlineMonitor(kHeartbeat), ErrorCode::kAlreadyStarted);
    ExpectError(monitor_->GetLogicMonitor(kHeartbeat), ErrorCode::kAlreadyStarted);
    ExpectError(monitor_->Start(), ErrorCode::kAlreadyStarted);
}

TEST_F(HealthMonitorImplTest, ConnectFailureReturnsSupervisorUnavailableAndAllowsANewStart)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__fail_silent");
    RecordProperty("TestType", "fault-injection");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Check that a failing Connect makes Start return kSupervisorUnavailable without a worker and that a "
        "second Start succeeds.");

    Create(kHealthyMin, kHealthyMax);
    const InSequence sequence;
    EXPECT_CALL(*client_, Connect()).WillOnce(Invoke([] {
        return score::Result<void>{score::MakeUnexpected(ErrorCode::kSupervisorUnavailable)};
    }));
    EXPECT_CALL(*client_, Connect()).WillOnce(Invoke(&Success));
    // The strict mock fails the test if a worker notifies alive before the second Connect.
    EXPECT_CALL(*client_, NotifyAlive()).Times(AnyNumber());
    auto taken = monitor_->GetHeartbeatMonitor(kHeartbeat);
    ASSERT_TRUE(taken.has_value());

    ExpectError(monitor_->Start(), ErrorCode::kSupervisorUnavailable);

    EXPECT_TRUE(monitor_->Start().has_value());
}

TEST_F(HealthMonitorImplTest, TakenMonitorAcceptsHeartbeatsAfterTheHealthMonitorIsDestroyed)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Check that a taken heartbeat monitor stays usable after the health monitor stopped its worker and "
        "was destroyed.");

    Create(kHealthyMin, kHealthyMax);
    ExpectConnectAndNotify();
    auto result = monitor_->GetHeartbeatMonitor(kHeartbeat);
    ASSERT_TRUE(result.has_value());
    auto taken = std::move(result).value();
    ASSERT_TRUE(monitor_->Start().has_value());

    monitor_.reset();

    taken->Heartbeat();
    taken->Heartbeat();
}

TEST_F(HealthMonitorImplTest, TakenLogicMonitorTransitionsAfterTheHealthMonitorIsDestroyed)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Check that a taken logic monitor stays usable after the health monitor stopped its worker and "
        "was destroyed.");

    CreateWithLogic();
    ExpectConnectAndNotify();
    auto heartbeat = monitor_->GetHeartbeatMonitor(kHeartbeat);
    ASSERT_TRUE(heartbeat.has_value());
    auto result = monitor_->GetLogicMonitor(kLogic);
    ASSERT_TRUE(result.has_value());
    auto taken = std::move(result).value();
    ASSERT_TRUE(monitor_->Start().has_value());

    monitor_.reset();

    const auto transition = taken->Transition(kStateB);
    ASSERT_TRUE(transition.has_value());
    EXPECT_EQ(transition.value(), kStateB);
}
