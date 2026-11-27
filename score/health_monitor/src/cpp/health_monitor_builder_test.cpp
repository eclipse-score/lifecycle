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

#include "score/health_monitor/health_monitor_builder.h"

#include "score/health_monitor/error.h"
#include "score/health_monitor/src/cpp/internal/test_helpers.h"

#include <gtest/gtest.h>

#include <pthread.h>
#include <unistd.h>
#include <array>
#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <ratio>
#include <type_traits>
#include <utility>

using namespace score::health_monitor;
using namespace score::health_monitor::details;
using namespace std::chrono_literals;

namespace
{

const MonitorTag kHeartbeat{"heartbeat"};
const MonitorTag kOtherHeartbeat{"other_heartbeat"};
const MonitorTag kDeadline{"deadline"};
const MonitorTag kOtherDeadline{"other_deadline"};
const MonitorTag kLogic{"logic"};
const DeadlineTag kDeadlineA{"deadline_a"};
const DeadlineTag kDeadlineB{"deadline_b"};

// Valid for the default internal processing cycle of 100 ms.
const TimeRange kRange{100ms, 200ms};

DeadlineMonitorBuilder OneDeadline()
{
    return DeadlineMonitorBuilder{}.AddDeadline(kDeadlineA, kRange);
}

HealthMonitorBuilder OneHeartbeat()
{
    return HealthMonitorBuilder{}.AddHeartbeatMonitor(kHeartbeat, HeartbeatMonitorBuilder{kRange});
}

LogicMonitorBuilder TwoStateLogic()
{
    const std::array<StateTag, 1> from_a{{StateTag{"b"}}};
    const std::array<StateTag, 1> from_b{{StateTag{"a"}}};
    return LogicMonitorBuilder{StateTag{"a"}}.AddState(StateTag{"a"}, from_a).AddState(StateTag{"b"}, from_b);
}

HealthMonitorBuilder WithCycles(const std::chrono::milliseconds internal, const std::chrono::milliseconds supervisor)
{
    return OneHeartbeat().WithInternalProcessingCycle(internal).WithSupervisorApiCycle(supervisor);
}

HealthMonitorBuilder WithThreadParameters(ThreadParameters&& parameters)
{
    return OneHeartbeat().WithThreadParameters(std::move(parameters));
}

}  // namespace

TEST(HealthMonitorBuilderTest, BuildsWithOneHeartbeatMonitor)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that the default cycles and one heartbeat monitor build a health monitor.");

    auto result = OneHeartbeat().Build();

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

TEST(HealthMonitorBuilderTest, AcceptsSupervisorCycleEqualToInternalCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a supervisor cycle equal to the internal cycle is accepted.");

    EXPECT_TRUE(WithCycles(100ms, 100ms).Build().has_value());
}

TEST(HealthMonitorBuilderTest, AcceptsSupervisorCycleTwiceTheInternalCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a supervisor cycle twice the internal cycle is accepted.");

    EXPECT_TRUE(WithCycles(100ms, 200ms).Build().has_value());
}

TEST(HealthMonitorBuilderTest, RejectsSupervisorCycleThatIsNoMultipleOfInternalCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Check that a supervisor cycle of 123 ms with an internal cycle of 100 ms fails with kInvalidArgument.");

    ExpectError(WithCycles(100ms, 123ms).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, RejectsSupervisorCycleShorterThanInternalCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description", "Check that a supervisor cycle below the internal cycle fails with kInvalidArgument.");

    ExpectError(WithCycles(100ms, 50ms).Build(), ErrorCode::kInvalidArgument);
}

template <typename Duration, typename = void>
struct AcceptsInternalCycle : std::false_type
{
};

template <typename Duration>
struct AcceptsInternalCycle<
    Duration,
    std::void_t<decltype(std::declval<HealthMonitorBuilder>().WithInternalProcessingCycle(std::declval<Duration>()))>>
    : std::true_type
{
};

template <typename Duration, typename = void>
struct AcceptsSupervisorCycle : std::false_type
{
};

template <typename Duration>
struct AcceptsSupervisorCycle<
    Duration,
    std::void_t<decltype(std::declval<HealthMonitorBuilder>().WithSupervisorApiCycle(std::declval<Duration>()))>>
    : std::true_type
{
};

TEST(HealthMonitorBuilderTest, SubMillisecondCyclesDoNotCompile)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_sub_ms");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description",
        "Check that the cycle setters do not take microseconds or fractional milliseconds, so a cycle that is not a "
        "whole number of milliseconds is rejected at compile time instead of being truncated.");

    using FractionalMilliseconds = std::chrono::duration<double, std::milli>;
    static_assert(AcceptsInternalCycle<std::chrono::milliseconds>::value, "Whole milliseconds must be accepted.");
    static_assert(AcceptsSupervisorCycle<std::chrono::milliseconds>::value, "Whole milliseconds must be accepted.");
    static_assert(!AcceptsInternalCycle<std::chrono::microseconds>::value, "Microseconds must be rejected.");
    static_assert(!AcceptsSupervisorCycle<std::chrono::microseconds>::value, "Microseconds must be rejected.");
    static_assert(!AcceptsInternalCycle<FractionalMilliseconds>::value, "Fractional milliseconds must be rejected.");
    static_assert(!AcceptsSupervisorCycle<FractionalMilliseconds>::value, "Fractional milliseconds must be rejected.");
}

TEST(HealthMonitorBuilderTest, RejectsZeroInternalCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that an internal cycle of zero fails with kInvalidArgument.");

    ExpectError(WithCycles(0ms, 100ms).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, RejectsNegativeInternalCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a negative internal cycle fails with kInvalidArgument.");

    ExpectError(WithCycles(-100ms, 100ms).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, RejectsZeroSupervisorCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a supervisor cycle of zero fails with kInvalidArgument.");

    ExpectError(WithCycles(100ms, 0ms).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, RejectsNegativeSupervisorCycle)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a negative supervisor cycle fails with kInvalidArgument.");

    ExpectError(WithCycles(100ms, -100ms).Build(), ErrorCode::kInvalidArgument);
}

namespace
{

/// @brief Longest cycle that `std::chrono::steady_clock::duration` holds.
const std::chrono::milliseconds kClockLimit{
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::duration::max())};

/// @brief One heartbeat monitor whose range passes the heartbeat check for an internal cycle of `cycle`.
HealthMonitorBuilder WithLongCycles(const std::chrono::milliseconds cycle)
{
    return HealthMonitorBuilder{}
        .AddHeartbeatMonitor(kHeartbeat, HeartbeatMonitorBuilder{TimeRange{cycle, cycle}})
        .WithInternalProcessingCycle(cycle)
        .WithSupervisorApiCycle(cycle);
}

}  // namespace

TEST(HealthMonitorBuilderTest, RejectsSupervisorCycleBeyondTheClockRange)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Check that the longest millisecond supervisor cycle with an internal cycle of 1 ms fails with "
        "kInvalidArgument.");

    ExpectError(WithCycles(1ms, std::chrono::milliseconds::max()).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, RejectsInternalCycleBeyondTheClockRange)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description", "Check that cycles 1 ms longer than the steady clock holds fail with kInvalidArgument.");

    ExpectError(WithLongCycles(kClockLimit + 1ms).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, AcceptsCyclesAtTheClockLimit)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that cycles of the longest duration the steady clock holds are accepted.");

    EXPECT_TRUE(WithLongCycles(kClockLimit).Build().has_value());
}

TEST(HealthMonitorBuilderTest, RejectsBuildWithoutMonitors)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_no_monitor");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that a builder without any monitor fails with kNoMonitors.");

    ExpectError(HealthMonitorBuilder{}.Build(), ErrorCode::kNoMonitors);
}

TEST(HealthMonitorBuilderTest, CycleErrorComesBeforeNoMonitors)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_cycle_multiple");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that an invalid cycle is reported before a missing monitor.");

    ExpectError(HealthMonitorBuilder{}.WithInternalProcessingCycle(0ms).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, RejectsRepeatedHeartbeatTag)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that two heartbeat monitors with the same tag fail with kInvalidArgument.");

    auto builder = OneHeartbeat().AddHeartbeatMonitor(kHeartbeat, HeartbeatMonitorBuilder{kRange});

    ExpectError(std::move(builder).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, RejectsRepeatedHeartbeatTagWithAnotherTagBetween)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description", "Check that a repeated heartbeat tag with another monitor between fails with kInvalidArgument.");

    auto builder = OneHeartbeat()
                       .AddHeartbeatMonitor(kOtherHeartbeat, HeartbeatMonitorBuilder{kRange})
                       .AddHeartbeatMonitor(kHeartbeat, HeartbeatMonitorBuilder{kRange});

    ExpectError(std::move(builder).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, AcceptsDistinctHeartbeatTags)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that two heartbeat monitors with different tags build a health monitor.");

    auto builder = OneHeartbeat().AddHeartbeatMonitor(kOtherHeartbeat, HeartbeatMonitorBuilder{kRange});

    EXPECT_TRUE(std::move(builder).Build().has_value());
}

TEST(HealthMonitorBuilderTest, BuildsWithOneDeadlineMonitor)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_no_monitor");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that a heartbeat monitor and a deadline monitor with one deadline build.");

    auto result = OneHeartbeat().AddDeadlineMonitor(kDeadline, OneDeadline()).Build();

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

TEST(HealthMonitorBuilderTest, BuildsWithOneLogicMonitor)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_no_monitor");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("Description", "Check that a logic monitor alone counts as a monitor and builds a health monitor.");

    auto result = HealthMonitorBuilder{}.AddLogicMonitor(kLogic, TwoStateLogic()).Build();

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

TEST(HealthMonitorBuilderTest, LogicGraphErrorPropagates)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_cfg_undef_target");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description", "Check that a logic state that lists an undefined target fails with kInvalidArgument.");

    const std::array<StateTag, 1> allowed{{StateTag{"missing"}}};
    auto builder = HealthMonitorBuilder{}.AddLogicMonitor(
        kLogic, LogicMonitorBuilder{StateTag{"a"}}.AddState(StateTag{"a"}, allowed));

    ExpectError(std::move(builder).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, AcceptsHeartbeatAndLogicMonitorWithDistinctTags)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that a heartbeat monitor and a logic monitor with different tags build.");

    auto builder = OneHeartbeat().AddLogicMonitor(kLogic, TwoStateLogic());

    EXPECT_TRUE(std::move(builder).Build().has_value());
}

TEST(HealthMonitorBuilderTest, RejectsLogicMonitorWithTheTagOfAHeartbeatMonitor)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description", "Check that a heartbeat and a logic monitor with one tag fail with kInvalidArgument.");

    auto builder = OneHeartbeat().AddLogicMonitor(kHeartbeat, TwoStateLogic());

    ExpectError(std::move(builder).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, RejectsRepeatedLogicTag)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that two logic monitors with the same tag fail with kInvalidArgument.");

    auto builder =
        HealthMonitorBuilder{}.AddLogicMonitor(kLogic, TwoStateLogic()).AddLogicMonitor(kLogic, TwoStateLogic());

    ExpectError(std::move(builder).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, LogicMonitorKeepsACopyOfTheAllowedStates)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__lg_invalid_trans");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty(
        "Description", "Check that an allowed transition works although the array given to AddState is gone at Build.");

    auto builder = [] {
        const std::array<StateTag, 1> from_a{{StateTag{"b"}}};
        const std::array<StateTag, 1> from_b{{StateTag{"a"}}};
        return HealthMonitorBuilder{}.AddLogicMonitor(
            kLogic, LogicMonitorBuilder{StateTag{"a"}}.AddState(StateTag{"a"}, from_a).AddState(StateTag{"b"}, from_b));
    }();
    auto health_monitor = std::move(builder).Build();
    ASSERT_TRUE(health_monitor.has_value());
    auto logic = health_monitor.value()->GetLogicMonitor(kLogic);
    ASSERT_TRUE(logic.has_value());

    const auto transition = logic.value()->Transition(StateTag{"b"});

    ASSERT_TRUE(transition.has_value());
    EXPECT_EQ(transition.value(), StateTag{"b"});
}

TEST(HealthMonitorBuilderTest, DeadlineMonitorAloneCountsAsAMonitor)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cfg_no_monitor");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty(
        "Description", "Check that a deadline monitor with one deadline alone counts as a monitor and builds.");

    auto result = HealthMonitorBuilder{}.AddDeadlineMonitor(kDeadline, OneDeadline()).Build();

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value(), nullptr);
}

TEST(HealthMonitorBuilderTest, RejectsDeadlineMonitorWithoutDeadlines)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_cfg");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that a deadline monitor without a deadline fails with kInvalidArgument.");

    auto builder = OneHeartbeat().AddDeadlineMonitor(kDeadline, DeadlineMonitorBuilder{});

    ExpectError(std::move(builder).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, RejectsDeadlineMonitorWithoutDeadlinesAlone)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__dl_cfg");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Check that a deadline monitor without a deadline fails with kInvalidArgument and not "
        "kNoMonitors.");

    auto builder = HealthMonitorBuilder{}.AddDeadlineMonitor(kDeadline, DeadlineMonitorBuilder{});

    ExpectError(std::move(builder).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, RejectsRepeatedDeadlineTag)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__deadline_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description", "Check that two deadlines with the same tag in one monitor fail with kInvalidArgument.");

    auto builder = HealthMonitorBuilder{}.AddDeadlineMonitor(
        kDeadline, DeadlineMonitorBuilder{}.AddDeadline(kDeadlineA, kRange).AddDeadline(kDeadlineA, kRange));

    ExpectError(std::move(builder).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, AcceptsDistinctDeadlineTags)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__deadline_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that two deadlines with different tags in one monitor build.");

    auto builder = HealthMonitorBuilder{}.AddDeadlineMonitor(
        kDeadline, DeadlineMonitorBuilder{}.AddDeadline(kDeadlineA, kRange).AddDeadline(kDeadlineB, kRange));

    EXPECT_TRUE(std::move(builder).Build().has_value());
}

TEST(HealthMonitorBuilderTest, BuiltDeadlineMonitorHandsOutEachAddedDeadline)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__deadline_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty(
        "Description", "Check that the built deadline monitor hands out each added deadline and no other deadline.");

    auto result =
        HealthMonitorBuilder{}
            .AddDeadlineMonitor(
                kDeadline, DeadlineMonitorBuilder{}.AddDeadline(kDeadlineA, kRange).AddDeadline(kDeadlineB, kRange))
            .Build();
    ASSERT_TRUE(result.has_value());
    auto deadline_monitor = result.value()->GetDeadlineMonitor(kDeadline);
    ASSERT_TRUE(deadline_monitor.has_value());

    EXPECT_TRUE(deadline_monitor.value()->GetDeadline(kDeadlineA).has_value());
    EXPECT_TRUE(deadline_monitor.value()->GetDeadline(kDeadlineB).has_value());
    ExpectError(deadline_monitor.value()->GetDeadline(DeadlineTag{"unknown"}), ErrorCode::kUnknownTag);
}

TEST(HealthMonitorBuilderTest, RejectsDeadlineMonitorWithTheTagOfAHeartbeatMonitor)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description", "Check that a heartbeat and a deadline monitor with one tag fail with kInvalidArgument.");

    auto builder = OneHeartbeat().AddDeadlineMonitor(kHeartbeat, OneDeadline());

    ExpectError(std::move(builder).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, RejectsDeadlineMonitorWithTheTagOfALogicMonitor)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that a logic and a deadline monitor with one tag fail with kInvalidArgument.");

    auto builder =
        HealthMonitorBuilder{}.AddLogicMonitor(kLogic, TwoStateLogic()).AddDeadlineMonitor(kLogic, OneDeadline());

    ExpectError(std::move(builder).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, RejectsRepeatedDeadlineMonitorTag)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that two deadline monitors with the same tag fail with kInvalidArgument.");

    auto builder = HealthMonitorBuilder{}
                       .AddDeadlineMonitor(kDeadline, OneDeadline())
                       .AddDeadlineMonitor(kDeadline, OneDeadline());

    ExpectError(std::move(builder).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderTest, AcceptsDeadlineHeartbeatAndLogicMonitorWithDistinctTags)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__monitor_id");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("Description", "Check that two deadline monitors, a heartbeat and a logic monitor build.");

    auto builder = OneHeartbeat()
                       .AddLogicMonitor(kLogic, TwoStateLogic())
                       .AddDeadlineMonitor(kDeadline, OneDeadline())
                       .AddDeadlineMonitor(kOtherDeadline, OneDeadline());

    EXPECT_TRUE(std::move(builder).Build().has_value());
}

TEST(HealthMonitorBuilderTest, HeartbeatRangeErrorPropagates)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__hb_cfg");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Check that a heartbeat range whose minimum is too short for the internal cycle fails with kInvalidArgument.");

    // 2 * 10 ms does not exceed the default internal cycle of 100 ms.
    auto builder =
        HealthMonitorBuilder{}.AddHeartbeatMonitor(kHeartbeat, HeartbeatMonitorBuilder{TimeRange{10ms, 20ms}});

    ExpectError(std::move(builder).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderThreadParametersTest, AcceptsFifoPriorityAtMinimum)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that the lowest FIFO priority is accepted.");

    const std::int32_t priority{SchedulerPolicyPriorityMin(SchedulerPolicy::kFifo)};

    EXPECT_TRUE(WithThreadParameters(ThreadParameters{}.WithSchedulerParameters({SchedulerPolicy::kFifo, priority}))
                    .Build()
                    .has_value());
}

TEST(HealthMonitorBuilderThreadParametersTest, AcceptsFifoPriorityAtMaximum)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that the highest FIFO priority is accepted.");

    const std::int32_t priority{SchedulerPolicyPriorityMax(SchedulerPolicy::kFifo)};

    EXPECT_TRUE(WithThreadParameters(ThreadParameters{}.WithSchedulerParameters({SchedulerPolicy::kFifo, priority}))
                    .Build()
                    .has_value());
}

TEST(HealthMonitorBuilderThreadParametersTest, RejectsFifoPriorityBelowMinimum)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a FIFO priority one below the minimum fails with kInvalidArgument.");

    const std::int32_t priority{SchedulerPolicyPriorityMin(SchedulerPolicy::kFifo) - 1};

    ExpectError(
        WithThreadParameters(ThreadParameters{}.WithSchedulerParameters({SchedulerPolicy::kFifo, priority})).Build(),
        ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderThreadParametersTest, RejectsFifoPriorityAboveMaximum)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a FIFO priority one above the maximum fails with kInvalidArgument.");

    const std::int32_t priority{SchedulerPolicyPriorityMax(SchedulerPolicy::kFifo) + 1};

    ExpectError(
        WithThreadParameters(ThreadParameters{}.WithSchedulerParameters({SchedulerPolicy::kFifo, priority})).Build(),
        ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderThreadParametersTest, AcceptsStackSizeAtMinimum)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that a stack size of PTHREAD_STACK_MIN is accepted.");

    const auto stack_size = static_cast<std::size_t>(PTHREAD_STACK_MIN);

    EXPECT_TRUE(WithThreadParameters(ThreadParameters{}.WithStackSize(stack_size)).Build().has_value());
}

TEST(HealthMonitorBuilderThreadParametersTest, RejectsStackSizeBelowMinimum)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description", "Check that a stack size one byte below PTHREAD_STACK_MIN fails with kInvalidArgument.");

    const auto stack_size = static_cast<std::size_t>(PTHREAD_STACK_MIN) - 1U;

    ExpectError(
        WithThreadParameters(ThreadParameters{}.WithStackSize(stack_size)).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderThreadParametersTest, AcceptsLastCpuIndex)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("Description", "Check that the highest existing CPU index is accepted.");

    const long cpu_count{::sysconf(_SC_NPROCESSORS_ONLN)};
    ASSERT_GT(cpu_count, 0);
    const std::array<std::size_t, 1> cpus{{static_cast<std::size_t>(cpu_count - 1)}};

    EXPECT_TRUE(WithThreadParameters(ThreadParameters{}.WithAffinity(cpus)).Build().has_value());
}

TEST(HealthMonitorBuilderThreadParametersTest, RejectsCpuIndexEqualToCpuCount)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description", "Check that a CPU index equal to the number of online CPUs fails with kInvalidArgument.");

    const long cpu_count{::sysconf(_SC_NPROCESSORS_ONLN)};
    ASSERT_GT(cpu_count, 0);
    const std::array<std::size_t, 1> cpus{{static_cast<std::size_t>(cpu_count)}};

    ExpectError(WithThreadParameters(ThreadParameters{}.WithAffinity(cpus)).Build(), ErrorCode::kInvalidArgument);
}

TEST(HealthMonitorBuilderThreadParametersTest, RejectsOneInvalidCpuAmongValidOnes)
{
    RecordProperty("PartiallyVerifies", "comp_req__health_monitor__cyclic_eval");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Check that one CPU index beyond the CPU count among valid indices fails with kInvalidArgument.");

    const long cpu_count{::sysconf(_SC_NPROCESSORS_ONLN)};
    ASSERT_GT(cpu_count, 0);
    const std::array<std::size_t, 2> cpus{{0U, static_cast<std::size_t>(cpu_count)}};

    ExpectError(WithThreadParameters(ThreadParameters{}.WithAffinity(cpus)).Build(), ErrorCode::kInvalidArgument);
}
