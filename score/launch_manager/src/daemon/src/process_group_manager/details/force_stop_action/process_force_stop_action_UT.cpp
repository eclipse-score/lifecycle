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

#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/process_force_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/mock_iprocess.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace ::testing;

const ProcessHandle mock_handle = ProcessHandle{42};

TEST(ProcessForceStopActionTest, ForceStopCallsForceTerminationWithPid)
{
    StrictMock<osal::MockIProcess> launcher;
    const ProcessForceStopAction process_force_stop_action(launcher);

    EXPECT_CALL(launcher, forceTermination(mock_handle.pid)).WillOnce(Return(osal::OsalReturnType::kSuccess));

    static_cast<void>(process_force_stop_action.forceStop(cpp::stop_token{}, mock_handle));
}

TEST(ProcessForceStopActionTest, ForceStopSucceedsReturnsSuccess)
{
    NiceMock<osal::MockIProcess> launcher;
    const ProcessForceStopAction process_force_stop_action(launcher);

    ON_CALL(launcher, forceTermination(_)).WillByDefault(Return(osal::OsalReturnType::kSuccess));

    const Result<void> result = process_force_stop_action.forceStop(cpp::stop_token{}, mock_handle);

    EXPECT_TRUE(result.has_value());
}

TEST(ProcessForceStopActionTest, ForceStopFailsReturnsError)
{
    NiceMock<osal::MockIProcess> launcher;
    const ProcessForceStopAction process_force_stop_action(launcher);

    ON_CALL(launcher, forceTermination(_)).WillByDefault(Return(osal::OsalReturnType::kFail));

    const Result<void> result = process_force_stop_action.forceStop(cpp::stop_token{}, mock_handle);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(ProcessForceStopActionTest, ForceStopWithInvalidatedHandleDoesNotCallLauncher)
{
    StrictMock<osal::MockIProcess> launcher;
    const ProcessForceStopAction process_force_stop_action(launcher);
    ProcessHandle handle = mock_handle;
    handle.pid = -1;

    const Result<void> result = process_force_stop_action.forceStop(cpp::stop_token{}, handle);

    EXPECT_TRUE(result.has_value());
}

TEST(ProcessForceStopActionTest, ForceStopWithEmptyHandleAborts)
{
    NiceMock<osal::MockIProcess> launcher;
    const ProcessForceStopAction process_force_stop_action(launcher);

    EXPECT_DEATH(
        static_cast<void>(process_force_stop_action.forceStop(cpp::stop_token{}, EmptyHandle{})), "Unreachable_Code");
}

}  // namespace
}  // namespace score::mw::lifecycle::internal
