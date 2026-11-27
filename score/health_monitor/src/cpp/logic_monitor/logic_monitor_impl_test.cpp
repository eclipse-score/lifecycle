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

#include "score/health_monitor/src/cpp/logic_monitor/logic_monitor_impl.h"

#include "score/health_monitor/error.h"

#include "score/health_monitor/src/cpp/internal/test_helpers.h"
#include "score/mw/log/recorder_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace score::health_monitor;
using namespace score::health_monitor::details;

namespace
{

using Monitor = LogicMonitorImpl;
using TimePoint = std::chrono::steady_clock::time_point;

const MonitorTag kTag{"logic"};
const StateTag kS1{"s1"};
const StateTag kS2{"s2"};
const StateTag kS3{"s3"};
const StateTag kOther{"other"};
const TimePoint kStart{};

/// @brief Graph s1 to s2 to s3, where s3 can return to s1 or stay in s3, and s1 and s2 cannot stay.
std::vector<StateNode> MakeStates()
{
    std::vector<StateNode> states;
    states.push_back(StateNode{kS1, {kS2}});
    states.push_back(StateNode{kS2, {kS3}});
    states.push_back(StateNode{kS3, {kS1, kS3}});
    return states;
}

std::shared_ptr<Monitor> MakeMonitor(const StateTag& initial_state = kS1)
{
    auto result = Monitor::Create(kTag, initial_state, MakeStates());
    EXPECT_TRUE(result.has_value());
    return result.has_value() ? std::move(result).value() : nullptr;
}

}  // namespace

TEST(LogicMonitorImplCreateTest, CreatesMonitorInTheInitialState)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_initial");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that a valid graph creates a monitor whose state is the initial state.");

    const auto monitor = MakeMonitor(kS1);

    ASSERT_NE(monitor, nullptr);
    const auto state = monitor->State();
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state.value(), kS1);
}

TEST(LogicMonitorImplCreateTest, CreatesMonitorInAnInitialStateThatIsNotTheFirstNode)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_initial");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that the initial state can be any node and not only the first one.");

    const auto monitor = MakeMonitor(kS3);

    ASSERT_NE(monitor, nullptr);
    const auto state = monitor->State();
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state.value(), kS3);
}

TEST(LogicMonitorImplCreateTest, RejectsNoStates)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_cfg_no_state");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that an empty state list fails with kInvalidArgument.");

    ExpectError(Monitor::Create(kTag, kS1, std::vector<StateNode>{}), ErrorCode::kInvalidArgument);
}

TEST(LogicMonitorImplCreateTest, RejectsRepeatedStateWithEqualTargets)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__state_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description", "Check that two nodes with the same state and the same targets fail with kInvalidArgument.");

    std::vector<StateNode> states;
    states.push_back(StateNode{kS1, {kS2}});
    states.push_back(StateNode{kS1, {kS2}});
    states.push_back(StateNode{kS2, {kS1}});

    ExpectError(Monitor::Create(kTag, kS1, std::move(states)), ErrorCode::kInvalidArgument);
}

TEST(LogicMonitorImplCreateTest, RejectsRepeatedStateWithDifferentTargets)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__state_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description", "Check that two nodes with the same state and different targets fail with kInvalidArgument.");

    std::vector<StateNode> states;
    states.push_back(StateNode{kS1, {kS2}});
    states.push_back(StateNode{kS2, {kS1}});
    states.push_back(StateNode{kS1, {kS1}});

    ExpectError(Monitor::Create(kTag, kS1, std::move(states)), ErrorCode::kInvalidArgument);
}

TEST(LogicMonitorImplCreateTest, RejectsUndefinedTarget)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_cfg_undef_target");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that a node that lists a state without a node fails with kInvalidArgument.");

    std::vector<StateNode> states;
    states.push_back(StateNode{kS1, {kS2}});

    ExpectError(Monitor::Create(kTag, kS1, std::move(states)), ErrorCode::kInvalidArgument);
}

TEST(LogicMonitorImplCreateTest, RejectsUndefinedInitialState)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_cfg_no_initial");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that an initial state without a node fails with kInvalidArgument.");

    ExpectError(Monitor::Create(kTag, kOther, MakeStates()), ErrorCode::kInvalidArgument);
}

TEST(LogicMonitorImplTransitionTest, AllowedTransitionReturnsTargetAndMovesTheState)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_invalid_trans");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that a listed target is returned and becomes the current state.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);

    const auto result = monitor->Transition(kS2);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), kS2);
    ASSERT_TRUE(monitor->State().has_value());
    EXPECT_EQ(monitor->State().value(), kS2);
}

TEST(LogicMonitorImplTransitionTest, ChainOfAllowedTransitionsSucceeds)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_invalid_trans");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that the chain s1, s2, s3 follows the graph and ends in s3.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);

    ASSERT_TRUE(monitor->Transition(kS2).has_value());
    const auto result = monitor->Transition(kS3);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), kS3);
    ASSERT_TRUE(monitor->State().has_value());
    EXPECT_EQ(monitor->State().value(), kS3);
}

TEST(LogicMonitorImplTransitionTest, DefinedButNotAllowedTargetFailsWithInvalidTransition)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_invalid_trans");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Check that a state of the graph that the current state does not list fails with "
        "kInvalidTransition and fails the monitor.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);

    ExpectError(monitor->Transition(kS3), ErrorCode::kInvalidTransition);

    ExpectError(monitor->State(), ErrorCode::kInvalidState);
}

TEST(LogicMonitorImplTransitionTest, ListedSelfTransitionSucceeds)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_invalid_trans");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a state that lists itself can move to itself.");

    const auto monitor = MakeMonitor(kS3);
    ASSERT_NE(monitor, nullptr);

    const auto result = monitor->Transition(kS3);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), kS3);
    ASSERT_TRUE(monitor->State().has_value());
    EXPECT_EQ(monitor->State().value(), kS3);
}

TEST(LogicMonitorImplTransitionTest, UnlistedSelfTransitionFailsWithInvalidTransition)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_invalid_trans");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a state that does not list itself fails to move to itself.");

    const auto monitor = MakeMonitor(kS1);
    ASSERT_NE(monitor, nullptr);

    ExpectError(monitor->Transition(kS1), ErrorCode::kInvalidTransition);

    ExpectError(monitor->State(), ErrorCode::kInvalidState);
}

TEST(LogicMonitorImplTransitionTest, UnconfiguredStateFailsWithInvalidTransition)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_unknown_state");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Check that a state that the graph does not have fails with kInvalidTransition and "
        "fails the monitor.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);

    ExpectError(monitor->Transition(kOther), ErrorCode::kInvalidTransition);

    ExpectError(monitor->State(), ErrorCode::kInvalidState);
}

TEST(LogicMonitorImplTransitionTest, TransitionAfterFailureFailsWithInvalidState)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__lg_invalid_trans, comp_req__health_monitor__lg_unknown_state");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that a failed monitor rejects a further transition with kInvalidState.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);
    ExpectError(monitor->Transition(kS3), ErrorCode::kInvalidTransition);

    ExpectError(monitor->Transition(kOther), ErrorCode::kInvalidState);
}

TEST(LogicMonitorImplTransitionTest, TransitionToAnAllowedTargetAfterFailureFailsWithInvalidState)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__lg_invalid_trans, comp_req__health_monitor__lg_unknown_state");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description",
        "Check that a failed monitor stays failed even for a target that the old state "
        "lists.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);
    ExpectError(monitor->Transition(kS3), ErrorCode::kInvalidTransition);

    ExpectError(monitor->Transition(kS2), ErrorCode::kInvalidState);

    ExpectError(monitor->State(), ErrorCode::kInvalidState);
}

TEST(LogicMonitorImplStateTest, StateAfterFailureFailsWithInvalidState)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__lg_invalid_trans, comp_req__health_monitor__lg_unknown_state");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that State fails with kInvalidState after an unconfigured state.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);
    ExpectError(monitor->Transition(kOther), ErrorCode::kInvalidTransition);

    ExpectError(monitor->State(), ErrorCode::kInvalidState);
}

TEST(LogicMonitorImplEvaluateTest, HealthyMonitorPasses)
{
    RecordProperty(
        "PartiallyVerifies",
        "comp_req__health_monitor__lg_invalid_trans, comp_req__health_monitor__lg_unknown_state, "
        "comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that Evaluate returns true for a healthy monitor.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);
    ASSERT_TRUE(monitor->Transition(kS2).has_value());

    EXPECT_TRUE(monitor->Evaluate(kStart));
}

TEST(LogicMonitorImplEvaluateTest, FailsAfterAnInvalidTransition)
{
    RecordProperty(
        "PartiallyVerifies",
        "comp_req__health_monitor__lg_invalid_trans, comp_req__health_monitor__lg_unknown_state, "
        "comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that Evaluate returns false after an invalid transition.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);
    ExpectError(monitor->Transition(kS3), ErrorCode::kInvalidTransition);

    EXPECT_FALSE(monitor->Evaluate(kStart));
}

TEST(LogicMonitorImplEvaluateTest, FailsWhileTheInvalidTransitionIsLogged)
{
    RecordProperty(
        "PartiallyVerifies", "comp_req__health_monitor__lg_invalid_trans, comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description", "Check that Evaluate returns false while the warning about an invalid transition is logged.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);
    ::testing::NiceMock<score::mw::log::RecorderMock> recorder;
    bool healthy_during_log{true};
    // The recorder call stands in for a log backend that blocks while the worker evaluates.
    EXPECT_CALL(recorder, StartRecord(::testing::_, score::mw::log::LogLevel::kWarn))
        .WillOnce([&monitor, &healthy_during_log](const std::string_view, const score::mw::log::LogLevel) {
            healthy_during_log = monitor->Evaluate(kStart);
            return score::cpp::optional<score::mw::log::SlotHandle>{};
        });
    const ScopedLogRecorder scoped_recorder{recorder};

    ExpectError(monitor->Transition(kS3), ErrorCode::kInvalidTransition);

    EXPECT_FALSE(healthy_during_log);
}

TEST(LogicMonitorImplEvaluateTest, PassingEvaluationDoesNotLog)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check that Evaluate after a valid transition logs nothing.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);
    ASSERT_TRUE(monitor->Transition(kS2).has_value());
    ::testing::NiceMock<score::mw::log::RecorderMock> recorder;
    EXPECT_CALL(recorder, StartRecord(::testing::_, ::testing::_)).Times(0);
    const ScopedLogRecorder scoped_recorder{recorder};

    EXPECT_TRUE(monitor->Evaluate(kStart));
}

TEST(LogicMonitorImplEvaluateTest, FailsAfterATransitionToAnUnconfiguredState)
{
    RecordProperty(
        "PartiallyVerifies",
        "comp_req__health_monitor__lg_invalid_trans, comp_req__health_monitor__lg_unknown_state, "
        "comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that Evaluate returns false after a transition to an unconfigured state.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);
    ExpectError(monitor->Transition(kOther), ErrorCode::kInvalidTransition);

    EXPECT_FALSE(monitor->Evaluate(kStart));
}

TEST(LogicMonitorImplEvaluateTest, FailsInEveryLaterCall)
{
    RecordProperty(
        "PartiallyVerifies",
        "comp_req__health_monitor__lg_invalid_trans, comp_req__health_monitor__lg_unknown_state, "
        "comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that the failure stays latched and every Evaluate returns false.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);
    ExpectError(monitor->Transition(kS3), ErrorCode::kInvalidTransition);

    EXPECT_FALSE(monitor->Evaluate(kStart));
    EXPECT_FALSE(monitor->Evaluate(kStart + std::chrono::seconds{1}));
    EXPECT_FALSE(monitor->Evaluate(kStart + std::chrono::seconds{2}));
}

TEST(LogicMonitorImplEvaluateTest, EvaluateDoesNotChangeTheState)
{
    RecordProperty(
        "PartiallyVerifies",
        "comp_req__health_monitor__lg_invalid_trans, comp_req__health_monitor__lg_unknown_state, "
        "comp_req__health_monitor__detect_latency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that Evaluate leaves a healthy and a failed monitor as it was.");

    const auto monitor = MakeMonitor();
    ASSERT_NE(monitor, nullptr);
    EXPECT_TRUE(monitor->Evaluate(kStart));
    ASSERT_TRUE(monitor->State().has_value());
    EXPECT_EQ(monitor->State().value(), kS1);
    ExpectError(monitor->Transition(kS3), ErrorCode::kInvalidTransition);

    EXPECT_FALSE(monitor->Evaluate(kStart));

    ExpectError(monitor->State(), ErrorCode::kInvalidState);
}

TEST(LogicMonitorImplConcurrencyTest, EvaluatePassesBeforeTheInvalidTransitionAndFailsAfter)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description",
        "Check that Evaluate on one thread returns true while another thread makes "
        "valid transitions, and false after the invalid one.");

    std::vector<StateNode> states;
    states.push_back(StateNode{kS1, {kS2}});
    states.push_back(StateNode{kS2, {kS1}});
    auto created = Monitor::Create(kTag, kS1, std::move(states));
    ASSERT_TRUE(created.has_value());
    const auto monitor = std::move(created).value();
    const int transitions{20000};
    std::atomic<bool> invalid_started{false};
    std::atomic<bool> done{false};
    std::atomic<bool> all_valid_succeeded{true};
    std::thread worker{[&] {
        StateTag target{kS2};
        for (int i{0}; i < transitions; ++i)
        {
            if (!monitor->Transition(target).has_value())
            {
                all_valid_succeeded = false;
            }
            target = (i % 2 == 0) ? kS1 : kS2;
        }
        // The state is s1 or s2 here, and neither lists "other".
        invalid_started = true;
        ExpectError(monitor->Transition(kOther), ErrorCode::kInvalidTransition);
        done = true;
    }};

    while (!done.load())
    {
        // A failure needs the invalid transition, which starts only after the flag is set.
        if (!monitor->Evaluate(kStart))
        {
            EXPECT_TRUE(invalid_started.load());
        }
    }
    worker.join();

    EXPECT_TRUE(all_valid_succeeded.load());
    EXPECT_FALSE(monitor->Evaluate(kStart));
}
