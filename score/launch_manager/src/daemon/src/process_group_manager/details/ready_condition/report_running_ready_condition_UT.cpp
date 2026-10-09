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

const ProcessHandle mock_handle = ProcessHandle{42};

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

TEST(ReportRunningReadyConditionTest, WaitWithEmptyHandleAborts)
{
    NiceMock<osal::MockIProcess> launcher;
    const ReportRunningReadyCondition report_running_ready_condition(launcher, std::nullopt);

    EXPECT_DEATH(
        static_cast<void>(report_running_ready_condition.wait(cpp::stop_token{}, EmptyHandle{})), "Unreachable_Code");
}

}  // namespace
}  // namespace score::mw::lifecycle::internal
