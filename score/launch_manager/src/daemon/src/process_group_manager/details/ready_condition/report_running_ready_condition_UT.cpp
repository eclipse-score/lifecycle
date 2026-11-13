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

    EXPECT_CALL(launcher, waitForkRunning(mock_handle.sync, std::optional<std::chrono::milliseconds>(10ms)))
        .WillOnce(Return(osal::OsalReturnType::kSuccess));

    static_cast<void>(report_running_ready_condition.wait(cpp::stop_token{}, mock_handle));
}

TEST(ReportRunningReadyConditionTest, WaitSucceedsReturnsSuccess)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, std::nullopt);

    ON_CALL(launcher, waitForkRunning(_, _)).WillByDefault(Return(osal::OsalReturnType::kSuccess));

    const Result<void> result = report_running_ready_condition.wait(cpp::stop_token{}, mock_handle);

    EXPECT_TRUE(result.has_value());
}

TEST(ReportRunningReadyConditionTest, WaitFailsReturnsError)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, std::nullopt);

    ON_CALL(launcher, waitForkRunning(_, _)).WillByDefault(Return(osal::OsalReturnType::kFail));

    const Result<void> result = report_running_ready_condition.wait(cpp::stop_token{}, mock_handle);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

/// @brief Provides a ProcessHandle whose send semaphore is initialised, because
///        tryHandleTermination() posts on it.
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

TEST_F(ReportRunningReadyConditionTerminationTest, TerminationMakesSuccessfulWaitFail)
{
    RecordProperty(
        "Description",
        "Given the process terminated during startup. "
        "When waitForkRunning succeeds because tryHandleTermination woke it up. "
        "Then wait returns an error instead of reporting the process as running.");

    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, 10ms);
    ON_CALL(launcher, waitForkRunning(_, _)).WillByDefault(Return(osal::OsalReturnType::kSuccess));

    report_running_ready_condition.prepare();
    EXPECT_TRUE(report_running_ready_condition.tryHandleTermination(handle_, 1));
    const Result<void> result = report_running_ready_condition.wait(cpp::stop_token{}, handle_);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST_F(ReportRunningReadyConditionTerminationTest, PrepareClearsTerminationOfPreviousActivation)
{
    RecordProperty(
        "Description",
        "Given a termination was reported during a previous activation. "
        "When prepare is called for the next activation and the process reports running. "
        "Then wait returns success.");

    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, 10ms);
    ON_CALL(launcher, waitForkRunning(_, _)).WillByDefault(Return(osal::OsalReturnType::kSuccess));

    EXPECT_TRUE(report_running_ready_condition.tryHandleTermination(handle_, 1));
    report_running_ready_condition.prepare();
    const Result<void> result = report_running_ready_condition.wait(cpp::stop_token{}, handle_);

    EXPECT_TRUE(result.has_value());
}

TEST(ReportRunningReadyConditionTest, WaitWithEmptyHandleAborts)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, std::nullopt);

    EXPECT_DEATH(
        static_cast<void>(report_running_ready_condition.wait(cpp::stop_token{}, EmptyHandle{})),
        "ReportRunningReadyCondition should only be called with a ProcessHandle");
}

TEST(ReportRunningReadyConditionTest, WaitWithNullSyncAborts)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, std::nullopt);

    EXPECT_DEATH(
        static_cast<void>(report_running_ready_condition.wait(cpp::stop_token{}, ProcessHandle{42, nullptr})),
        "ReportRunningReadyCondition should only be called with a ProcessHandle containing IPCCommsSync");
}

TEST(ReportRunningReadyConditionTest, TryHandleTerminationWithEmptyHandleAborts)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, std::nullopt);

    EXPECT_DEATH(
        static_cast<void>(report_running_ready_condition.tryHandleTermination(EmptyHandle{}, 0)),
        "ReportRunningReadyCondition should only be called with a ProcessHandle");
}

TEST(ReportRunningReadyConditionTest, TryHandleTerminationWithNullSyncAborts)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, std::nullopt);

    EXPECT_DEATH(
        static_cast<void>(report_running_ready_condition.tryHandleTermination(ProcessHandle{42, nullptr}, 0)),
        "ReportRunningReadyCondition should only be called with a ProcessHandle containing IPCCommsSync");
}

}  // namespace
}  // namespace score::mw::lifecycle::internal
