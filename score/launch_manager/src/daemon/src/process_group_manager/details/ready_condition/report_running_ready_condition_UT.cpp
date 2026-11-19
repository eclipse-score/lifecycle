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

#include "score/mw/launch_manager/process_group_manager/details/ready_condition/report_running_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/mock_iprocess.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <future>

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace ::testing;
using namespace std::chrono_literals;

alignas(osal::IpcCommsSync) std::array<std::byte, sizeof(osal::IpcCommsSync)> sync_buffer;

// The semaphores are never touched, since the launcher is mocked.
const ProcessHandle mock_handle = ProcessHandle{
    42,
    osal::IpcCommsP{reinterpret_cast<osal::IpcCommsSync*>(sync_buffer.data()), [](osal::IpcCommsSync*) {
                    }}};

TEST(ReportRunningReadyConditionTest, WaitCallsWaitForkRunningWithSyncAndTimeout)
{
    StrictMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, 10ms);

    EXPECT_CALL(launcher, waitForkRunning(mock_handle.sync, 10ms)).WillOnce(Return(osal::OsalReturnType::kSuccess));

    static_cast<void>(report_running_ready_condition.wait(cpp::stop_token{}, mock_handle));
}

TEST(ReportRunningReadyConditionTest, WaitSucceedsReturnsSuccess)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, 10ms);

    ON_CALL(launcher, waitForkRunning(_, _)).WillByDefault(Return(osal::OsalReturnType::kSuccess));

    const Result<void> result = report_running_ready_condition.wait(cpp::stop_token{}, mock_handle);

    EXPECT_TRUE(result.has_value());
}

TEST(ReportRunningReadyConditionTest, WaitFailsReturnsError)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, 10ms);

    ON_CALL(launcher, waitForkRunning(_, _)).WillByDefault(Return(osal::OsalReturnType::kFail));

    const Result<void> result = report_running_ready_condition.wait(cpp::stop_token{}, mock_handle);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(ReportRunningReadyConditionTest, WaitTimesOutReturnsTimeout)
{
    RecordProperty(
        "Description",
        "Given the process does not report kRunning within the timeout. "
        "When wait is called. "
        "Then wait returns kTimeout.");

    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, 10ms);
    ON_CALL(launcher, waitForkRunning(_, _)).WillByDefault(Return(osal::OsalReturnType::kTimeout));

    const Result<void> result = report_running_ready_condition.wait(cpp::stop_token{}, mock_handle);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kTimeout);
}

/// @brief Provides a ProcessHandle whose send semaphore is initialised, because
///        handleTermination() and stop requests post on it.
class ReportRunningReadyConditionTerminationTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ASSERT_EQ(sync_->send_sync_.init(0U, false), osal::OsalReturnType::kSuccess);
    }

    void TearDown() override
    {
        static_cast<void>(sync_->send_sync_.deinit());
    }

    alignas(osal::IpcCommsSync) std::array<std::byte, sizeof(osal::IpcCommsSync)> buffer_{};
    const osal::IpcCommsP sync_{reinterpret_cast<osal::IpcCommsSync*>(buffer_.data()), [](osal::IpcCommsSync*) {
                                }};
    const ProcessHandle handle_{42, sync_};
};

TEST_F(ReportRunningReadyConditionTerminationTest, HandleTerminationPostsExactlyOnce)
{
    RecordProperty(
        "Description",
        "Given a process which terminated during startup. "
        "When handleTermination is called. "
        "Then it posts exactly once, which completes the kRunning handshake only if the process "
        "already posted its kRunning report itself.");

    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, 10ms);

    report_running_ready_condition.handleTermination(handle_, 0);

    EXPECT_EQ(sync_->send_sync_.timedWait(0ms), osal::OsalReturnType::kSuccess);
    EXPECT_EQ(sync_->send_sync_.timedWait(0ms), osal::OsalReturnType::kTimeout);
}

TEST_F(ReportRunningReadyConditionTerminationTest, TerminationDoesNotOverrideSuccessfulHandshake)
{
    RecordProperty(
        "Description",
        "Given a process which reported kRunning and terminated afterwards. "
        "When the kRunning handshake succeeds. "
        "Then wait returns success, because the process did report kRunning.");

    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, 10ms);
    ON_CALL(launcher, waitForkRunning(_, _)).WillByDefault(Return(osal::OsalReturnType::kSuccess));

    report_running_ready_condition.handleTermination(handle_, 0);
    const Result<void> result = report_running_ready_condition.wait(cpp::stop_token{}, handle_);

    EXPECT_TRUE(result.has_value());
}

/// @brief Simulates ProcessLauncher::waitForkRunning(): blocks on the real semaphore of the given sync
///        and succeeds only if the process reported kRunning, which never happens in these tests.
osal::OsalReturnType blockOnSemaphore(osal::IpcCommsP sync, std::chrono::milliseconds timeout)
{
    const osal::OsalReturnType result = sync->send_sync_.timedWait(timeout);
    return (result == osal::OsalReturnType::kSuccess) ? osal::OsalReturnType::kFail : result;
}

/// @brief A timeout which never expires during a test run. A wait which is not woken up shows up as a
///        hanging test, never as a flake.
constexpr std::chrono::milliseconds kNeverExpires{std::chrono::hours{1}};

TEST_F(ReportRunningReadyConditionTerminationTest, StoppedTokenWakesWait)
{
    RecordProperty(
        "Description",
        "Given an already stopped stop token and a process which never reports kRunning. "
        "When wait is called with a timeout which never expires. "
        "Then wait returns kGeneralError without waiting for the timeout.");

    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, kNeverExpires);
    ON_CALL(launcher, waitForkRunning(_, _)).WillByDefault(Invoke(blockOnSemaphore));
    cpp::stop_source stop_source;
    static_cast<void>(stop_source.request_stop());

    const Result<void> result = report_running_ready_condition.wait(stop_source.get_token(), handle_);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST_F(ReportRunningReadyConditionTerminationTest, StopRequestedMidWaitWakesWait)
{
    RecordProperty(
        "Description",
        "Given wait is blocking on another thread for a process which never reports kRunning. "
        "When a stop is requested. "
        "Then wait is unblocked and returns kGeneralError.");

    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, kNeverExpires);
    std::promise<void> waiting;
    ON_CALL(launcher, waitForkRunning(_, _))
        .WillByDefault(Invoke([&waiting](osal::IpcCommsP sync, std::chrono::milliseconds timeout) {
            waiting.set_value();
            return blockOnSemaphore(std::move(sync), timeout);
        }));
    cpp::stop_source stop_source;

    auto future = std::async(std::launch::async, [&] {
        return report_running_ready_condition.wait(stop_source.get_token(), handle_);
    });
    waiting.get_future().wait();
    static_cast<void>(stop_source.request_stop());

    const Result<void> result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(ReportRunningReadyConditionTest, WaitWithEmptyHandleAborts)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, 10ms);

    EXPECT_DEATH(
        static_cast<void>(report_running_ready_condition.wait(cpp::stop_token{}, EmptyHandle{})),
        "ReportRunningReadyCondition should only be called with a ProcessHandle");
}

TEST(ReportRunningReadyConditionTest, WaitWithNullSyncAborts)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, 10ms);

    EXPECT_DEATH(
        static_cast<void>(report_running_ready_condition.wait(cpp::stop_token{}, ProcessHandle{42, nullptr})),
        "ReportRunningReadyCondition should only be called with a ProcessHandle containing IPCCommsSync");
}

TEST(ReportRunningReadyConditionTest, HandleTerminationWithEmptyHandleAborts)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, 10ms);

    EXPECT_DEATH(
        report_running_ready_condition.handleTermination(EmptyHandle{}, 0),
        "ReportRunningReadyCondition should only be called with a ProcessHandle");
}

TEST(ReportRunningReadyConditionTest, HandleTerminationWithNullSyncAborts)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, 10ms);

    EXPECT_DEATH(
        report_running_ready_condition.handleTermination(ProcessHandle{42, nullptr}, 0),
        "ReportRunningReadyCondition should only be called with a ProcessHandle containing IPCCommsSync");
}

}  // namespace
}  // namespace score::mw::lifecycle::internal
