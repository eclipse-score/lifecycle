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

#include "score/mw/launch_manager/osal/mock_ifile_waiter.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/file_ready_condition.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace score::mw::lifecycle::internal
{
namespace
{

using namespace ::testing;
using namespace std::chrono_literals;

constexpr score::safecpp::zstring_view kFilePath = "/some/path";

TEST(FileReadyConditionTest, WaitCallsWaitForFile)
{
    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 10ms, 5ms);

    EXPECT_CALL(
        file_waiter, waitForFile(Eq(kFilePath), Eq(configuration::FileExistenceState::Exists), Eq(10ms), Eq(5ms), _))
        .WillOnce(Return(osal::OsalReturnType::kSuccess));

    static_cast<void>(file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{}));
}

TEST(FileReadyConditionTest, WaitSucceedsReturnsSuccess)
{
    NiceMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 10ms, 5ms);

    ON_CALL(file_waiter, waitForFile(_, _, _, _, _)).WillByDefault(Return(osal::OsalReturnType::kSuccess));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{});

    EXPECT_TRUE(result.has_value());
}

TEST(FileReadyConditionTest, WaitFailsReturnsError)
{
    NiceMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 10ms, 5ms);

    ON_CALL(file_waiter, waitForFile(_, _, _, _, _)).WillByDefault(Return(osal::OsalReturnType::kFail));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(FileReadyConditionTest, WaitTimesOutReturnsError)
{
    NiceMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 10ms, 5ms);

    ON_CALL(file_waiter, waitForFile(_, _, _, _, _)).WillByDefault(Return(osal::OsalReturnType::kTimeout));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{});

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(FileReadyConditionTest, WaitAcceptsVariousHandles)
{
    NiceMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 10ms, 5ms);

    ON_CALL(file_waiter, waitForFile(_, _, _, _, _)).WillByDefault(Return(osal::OsalReturnType::kSuccess));

    const Result<void> result_empty = file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{});
    EXPECT_TRUE(result_empty.has_value());

    const Result<void> result_process = file_ready_condition.wait(cpp::stop_token{}, ProcessHandle{});
    EXPECT_TRUE(result_process.has_value());
}

}  // namespace
}  // namespace score::mw::lifecycle::internal
