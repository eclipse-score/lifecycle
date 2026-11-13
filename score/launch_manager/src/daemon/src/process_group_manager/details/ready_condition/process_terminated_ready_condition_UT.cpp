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

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace std::chrono_literals;

/// @brief A timeout which never expires during a test run. Tests which must not time out use it and
///        check the result instead of measuring time, so they cannot become flaky on slow machines.
///        A wait which is not woken up shows up as a wrong result or a hanging test, never as a flake.
constexpr std::chrono::milliseconds kNeverExpires{std::chrono::hours{1}};

class ProcessTerminatedReadyConditionTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("DerivationTechnique", "explorative-testing");
    }

    /// @brief Calls wait() on another thread and returns once that thread is about to enter wait().
    /// @details Whether a notification then arrives just before or during the wait cannot be observed
    ///          from outside without a test seam in the waiter. Both orders must succeed, so the test
    ///          result does not depend on it.
    std::future<Result<void>> waitOnOtherThread(
        const ProcessTerminatedReadyCondition& ready_condition,
        cpp::stop_token stop_token) const
    {
        std::promise<void> entering;
        std::future<void> entering_future = entering.get_future();
        auto result = std::async(
            std::launch::async, [this, &ready_condition, stop_token, entering = std::move(entering)]() mutable {
                entering.set_value();
                return ready_condition.wait(stop_token, process_handle_);
            });
        entering_future.wait();
        return result;
    }

    // No sync is needed, since this ready condition does not use the IPC channel.
    const ProcessHandle process_handle_{42, nullptr};
};

TEST_F(ProcessTerminatedReadyConditionTest, WaitTimesOutReturnsError)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given no termination is reported. "
        "When wait is called. "
        "Then wait returns kTimeout after the timeout.");

    const ProcessTerminatedReadyCondition ready_condition(10ms);

    const Result<void> result = ready_condition.wait(cpp::stop_token{}, process_handle_);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kTimeout);
}

TEST_F(ProcessTerminatedReadyConditionTest, TerminationBeforeWaitReturnsSuccess)
{
    RecordProperty(
        "Description",
        "Given the process terminated successfully before wait is called. "
        "When wait is called. "
        "Then the termination is not lost and wait returns success.");

    const ProcessTerminatedReadyCondition ready_condition(0ms);

    EXPECT_TRUE(ready_condition.tryHandleTermination(process_handle_, 0));
    const Result<void> result = ready_condition.wait(cpp::stop_token{}, process_handle_);

    EXPECT_TRUE(result.has_value());
}

TEST_F(ProcessTerminatedReadyConditionTest, TerminationDuringWaitReturnsSuccess)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given wait is blocking on another thread. "
        "When a successful termination is reported. "
        "Then wait is unblocked and returns success.");

    const ProcessTerminatedReadyCondition ready_condition(kNeverExpires);

    auto future = waitOnOtherThread(ready_condition, cpp::stop_token{});
    EXPECT_TRUE(ready_condition.tryHandleTermination(process_handle_, 0));

    // Without the termination, the wait would not return in time, so success proves the wakeup.
    EXPECT_TRUE(future.get().has_value());
}

TEST_F(ProcessTerminatedReadyConditionTest, NonZeroExitStatusIsRejected)
{
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty(
        "Description",
        "Given the process terminated with a non-zero exit status. "
        "When tryHandleTermination is called. "
        "Then the termination is rejected and wait does not succeed.");

    const ProcessTerminatedReadyCondition ready_condition(10ms);

    EXPECT_FALSE(ready_condition.tryHandleTermination(process_handle_, 1));
    const Result<void> result = ready_condition.wait(cpp::stop_token{}, process_handle_);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kTimeout);
}

TEST_F(ProcessTerminatedReadyConditionTest, StoppedTokenReturnsErrorImmediately)
{
    RecordProperty(
        "Description",
        "Given an already stopped stop token. "
        "When wait is called with a timeout which never expires. "
        "Then wait returns kGeneralError, which shows it did not wait for the timeout.");

    const ProcessTerminatedReadyCondition ready_condition(kNeverExpires);
    cpp::stop_source stop_source;
    static_cast<void>(stop_source.request_stop());

    const Result<void> result = ready_condition.wait(stop_source.get_token(), process_handle_);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST_F(ProcessTerminatedReadyConditionTest, StopRequestedMidWaitReturnsError)
{
    RecordProperty(
        "Description",
        "Given wait is blocking on another thread. "
        "When a stop is requested. "
        "Then wait is unblocked and returns kGeneralError rather than kTimeout.");

    const ProcessTerminatedReadyCondition ready_condition(kNeverExpires);
    cpp::stop_source stop_source;

    auto future = waitOnOtherThread(ready_condition, stop_source.get_token());
    static_cast<void>(stop_source.request_stop());

    const Result<void> result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST_F(ProcessTerminatedReadyConditionTest, LateTerminationAfterTimeoutIsDiscardedByPrepare)
{
    RecordProperty(
        "Description",
        "Given a wait that timed out and a successful termination reported afterwards. "
        "When prepare is called for the next activation and wait is called again. "
        "Then the stale termination is discarded and wait returns kTimeout.");

    const ProcessTerminatedReadyCondition ready_condition(10ms);

    // First activation: the process does not terminate in time.
    ready_condition.prepare();
    EXPECT_FALSE(ready_condition.wait(cpp::stop_token{}, process_handle_).has_value());

    // The first process terminates successfully, but too late.
    EXPECT_TRUE(ready_condition.tryHandleTermination(process_handle_, 0));

    // Second activation: the new process has not terminated yet.
    ready_condition.prepare();
    const Result<void> result = ready_condition.wait(cpp::stop_token{}, process_handle_);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kTimeout);
}

TEST_F(ProcessTerminatedReadyConditionTest, TerminationAfterPrepareIsKept)
{
    RecordProperty(
        "Description",
        "Given prepare was called for a new activation. "
        "When the process terminates successfully before wait is called. "
        "Then the termination is kept and wait returns success.");

    const ProcessTerminatedReadyCondition ready_condition(0ms);

    ready_condition.prepare();
    EXPECT_TRUE(ready_condition.tryHandleTermination(process_handle_, 0));

    EXPECT_TRUE(ready_condition.wait(cpp::stop_token{}, process_handle_).has_value());
}

TEST_F(ProcessTerminatedReadyConditionTest, WaitWithEmptyHandleAborts)
{
    const ProcessTerminatedReadyCondition ready_condition(0ms);

    EXPECT_DEATH(
        static_cast<void>(ready_condition.wait(cpp::stop_token{}, EmptyHandle{})),
        "ProcessTerminatedReadyCondition should only be called with a ProcessHandle");
}

TEST_F(ProcessTerminatedReadyConditionTest, TryHandleTerminationWithEmptyHandleAborts)
{
    const ProcessTerminatedReadyCondition ready_condition(0ms);

    EXPECT_DEATH(
        static_cast<void>(ready_condition.tryHandleTermination(EmptyHandle{}, 0)),
        "ProcessTerminatedReadyCondition should only be called with a ProcessHandle");
}

}  // namespace
}  // namespace score::mw::lifecycle::internal
