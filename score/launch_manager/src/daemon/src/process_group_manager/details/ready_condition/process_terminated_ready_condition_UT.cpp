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

#include "score/mw/launch_manager/process_group_manager/details/ready_condition/process_terminated_ready_condition.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <gtest/gtest.h>
#include <chrono>
#include <future>
#include <thread>

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace std::chrono_literals;

// No sync is needed, since this ready condition does not use the IPC channel.
const ProcessHandle process_handle = ProcessHandle{42, nullptr};

TEST(ProcessTerminatedReadyConditionTest, WaitTimesOutReturnsError)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given no termination is reported. "
        "When wait is called. "
        "Then wait returns an error after the timeout.");

    const ProcessTerminatedReadyCondition ready_condition(10ms);

    const Result<void> result = ready_condition.wait(cpp::stop_token{}, process_handle);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(ProcessTerminatedReadyConditionTest, TerminationBeforeWaitReturnsSuccess)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given the process terminated successfully before wait is called. "
        "When wait is called. "
        "Then the termination is not lost and wait returns success.");

    const ProcessTerminatedReadyCondition ready_condition(0ms);

    EXPECT_TRUE(ready_condition.tryHandleTermination(process_handle, 0));
    const Result<void> result = ready_condition.wait(cpp::stop_token{}, process_handle);

    EXPECT_TRUE(result.has_value());
}

TEST(ProcessTerminatedReadyConditionTest, TerminationDuringWaitReturnsSuccess)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given wait is blocking. "
        "When a successful termination is reported from another thread. "
        "Then wait is unblocked and returns success.");

    const ProcessTerminatedReadyCondition ready_condition(5000ms);

    auto future = std::async(std::launch::async, [&ready_condition] {
        return ready_condition.wait(cpp::stop_token{}, process_handle);
    });

    // The termination may arrive before or during the wait; both must succeed.
    std::this_thread::sleep_for(10ms);
    EXPECT_TRUE(ready_condition.tryHandleTermination(process_handle, 0));

    ASSERT_EQ(future.wait_for(1000ms), std::future_status::ready);
    EXPECT_TRUE(future.get().has_value());
}

TEST(ProcessTerminatedReadyConditionTest, NonZeroExitStatusIsRejected)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given the process terminated with a non-zero exit status. "
        "When tryHandleTermination is called. "
        "Then the termination is rejected and wait does not succeed.");

    const ProcessTerminatedReadyCondition ready_condition(10ms);

    EXPECT_FALSE(ready_condition.tryHandleTermination(process_handle, 1));
    const Result<void> result = ready_condition.wait(cpp::stop_token{}, process_handle);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(ProcessTerminatedReadyConditionTest, StoppedTokenReturnsErrorImmediately)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given an already stopped stop token. "
        "When wait is called with a long timeout. "
        "Then wait returns an error without waiting for the timeout.");

    const ProcessTerminatedReadyCondition ready_condition(5000ms);
    cpp::stop_source stop_source;
    static_cast<void>(stop_source.request_stop());

    const auto start = std::chrono::steady_clock::now();
    const Result<void> result = ready_condition.wait(stop_source.get_token(), process_handle);
    const auto duration = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(result.has_value());
    EXPECT_LT(duration, 1000ms);
}

TEST(ProcessTerminatedReadyConditionTest, StopRequestedMidWaitReturnsError)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given wait is blocking. "
        "When a stop is requested. "
        "Then wait is unblocked and returns an error.");

    const ProcessTerminatedReadyCondition ready_condition(5000ms);
    cpp::stop_source stop_source;

    auto future = std::async(std::launch::async, [&ready_condition, token = stop_source.get_token()] {
        return ready_condition.wait(token, process_handle);
    });

    std::this_thread::sleep_for(10ms);
    static_cast<void>(stop_source.request_stop());

    ASSERT_EQ(future.wait_for(1000ms), std::future_status::ready);
    EXPECT_FALSE(future.get().has_value());
}

TEST(ProcessTerminatedReadyConditionTest, LateTerminationAfterTimeoutIsDiscardedByPrepare)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a wait that timed out and a successful termination reported afterwards. "
        "When prepare is called for the next activation and wait is called again. "
        "Then the stale termination is discarded and wait returns an error.");

    const ProcessTerminatedReadyCondition ready_condition(10ms);

    // First activation: the process does not terminate in time.
    ready_condition.prepare();
    EXPECT_FALSE(ready_condition.wait(cpp::stop_token{}, process_handle).has_value());

    // The first process terminates successfully, but too late.
    EXPECT_TRUE(ready_condition.tryHandleTermination(process_handle, 0));

    // Second activation: the new process has not terminated yet.
    ready_condition.prepare();
    const Result<void> result = ready_condition.wait(cpp::stop_token{}, process_handle);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(ProcessTerminatedReadyConditionTest, TerminationAfterPrepareIsKept)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given prepare was called for a new activation. "
        "When the process terminates successfully before wait is called. "
        "Then the termination is kept and wait returns success.");

    const ProcessTerminatedReadyCondition ready_condition(0ms);

    ready_condition.prepare();
    EXPECT_TRUE(ready_condition.tryHandleTermination(process_handle, 0));

    EXPECT_TRUE(ready_condition.wait(cpp::stop_token{}, process_handle).has_value());
}

TEST(ProcessTerminatedReadyConditionTest, WaitWithEmptyHandleAborts)
{
    const ProcessTerminatedReadyCondition ready_condition(0ms);

    EXPECT_DEATH(
        static_cast<void>(ready_condition.wait(cpp::stop_token{}, EmptyHandle{})),
        "ProcessTerminatedReadyCondition should only be called with a ProcessHandle");
}

TEST(ProcessTerminatedReadyConditionTest, TryHandleTerminationWithEmptyHandleAborts)
{
    const ProcessTerminatedReadyCondition ready_condition(0ms);

    EXPECT_DEATH(
        static_cast<void>(ready_condition.tryHandleTermination(EmptyHandle{}, 0)),
        "ProcessTerminatedReadyCondition should only be called with a ProcessHandle");
}

}  // namespace
}  // namespace score::mw::lifecycle::internal
