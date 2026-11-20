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
#include <condition_variable>
#include <future>
#include <mutex>

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
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms);

    EXPECT_CALL(
        file_waiter, waitForFile(Eq(kFilePath), Eq(configuration::FileExistenceState::Exists), Eq(10ms), Eq(5ms), _))
        .WillOnce(Return(osal::OsalReturnType::kSuccess));

    static_cast<void>(file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{}, 10ms));
}

TEST(FileReadyConditionTest, WaitSucceedsReturnsSuccess)
{
    NiceMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms);

    ON_CALL(file_waiter, waitForFile(_, _, _, _, _)).WillByDefault(Return(osal::OsalReturnType::kSuccess));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{}, 10ms);

    EXPECT_TRUE(result.has_value());
}

TEST(FileReadyConditionTest, WaitFailsReturnsError)
{
    NiceMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms);

    ON_CALL(file_waiter, waitForFile(_, _, _, _, _)).WillByDefault(Return(osal::OsalReturnType::kFail));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{}, 10ms);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(FileReadyConditionTest, WaitTimesOutReturnsTimeout)
{
    NiceMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms);

    ON_CALL(file_waiter, waitForFile(_, _, _, _, _)).WillByDefault(Return(osal::OsalReturnType::kTimeout));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{}, 10ms);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kTimeout);
}

TEST(FileReadyConditionTest, WaitAcceptsVariousHandles)
{
    NiceMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms);

    ON_CALL(file_waiter, waitForFile(_, _, _, _, _)).WillByDefault(Return(osal::OsalReturnType::kSuccess));

    const Result<void> result_empty = file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{}, 10ms);
    EXPECT_TRUE(result_empty.has_value());

    const Result<void> result_process = file_ready_condition.wait(cpp::stop_token{}, ProcessHandle{}, 10ms);
    EXPECT_TRUE(result_process.has_value());
}

/// @brief A timeout which never expires during a test run. A wait which is not ended shows up as a
///        hanging test, never as a flake.
constexpr std::chrono::milliseconds kNeverExpires{std::chrono::hours{1}};

/// @brief Simulates FileWaiter::waitForFile() for a file which never reaches its state: blocks until a
///        stop is requested on the token, then fails like the real implementation.
osal::OsalReturnType blockUntilStopped(
    score::safecpp::zstring_view,
    configuration::FileExistenceState,
    std::chrono::milliseconds,
    std::chrono::milliseconds,
    const cpp::stop_token& stop_token)
{
    std::mutex mutex;
    std::condition_variable cv;
    // Registered before locking: for an already stopped token, the callback runs right here.
    const cpp::stop_callback wake{stop_token, [&mutex, &cv] {
                                      const std::lock_guard<std::mutex> guard{mutex};
                                      cv.notify_all();
                                  }};
    std::unique_lock<std::mutex> lock{mutex};
    cv.wait(lock, [&stop_token] {
        return stop_token.stop_requested();
    });
    return osal::OsalReturnType::kFail;
}

TEST(FileReadyConditionTest, TerminationBeforeWaitSucceedsIfFileReachedState)
{
    RecordProperty(
        "Description",
        "Given the process terminated before wait is called. "
        "When wait is called and the file has reached its state. "
        "Then the final check succeeds and wait returns success.");

    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms);

    file_ready_condition.prepare();
    file_ready_condition.handleTermination(ProcessHandle{42, nullptr}, 0);

    InSequence sequence;
    EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(kNeverExpires), _, _)).WillOnce(Invoke(blockUntilStopped));
    EXPECT_CALL(file_waiter, waitForFile(Eq(kFilePath), Eq(configuration::FileExistenceState::Exists), Eq(0ms), _, _))
        .WillOnce(Return(osal::OsalReturnType::kSuccess));

    EXPECT_TRUE(file_ready_condition.wait(cpp::stop_token{}, ProcessHandle{42, nullptr}, kNeverExpires).has_value());
}

TEST(FileReadyConditionTest, TerminationFailsWaitIfFileDidNotReachState)
{
    RecordProperty(
        "Description",
        "Given the process terminated before wait is called. "
        "When wait is called and the file has not reached its state. "
        "Then wait returns kGeneralError without waiting for the timeout.");

    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms);

    file_ready_condition.prepare();
    file_ready_condition.handleTermination(ProcessHandle{42, nullptr}, 0);

    InSequence sequence;
    EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(kNeverExpires), _, _)).WillOnce(Invoke(blockUntilStopped));
    EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(0ms), _, _)).WillOnce(Return(osal::OsalReturnType::kTimeout));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, ProcessHandle{42, nullptr}, kNeverExpires);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(FileReadyConditionTest, TerminationDuringWaitEndsWait)
{
    RecordProperty(
        "Description",
        "Given wait is blocking on another thread. "
        "When the process terminates. "
        "Then the wait ends and the file is checked once more.");

    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms);
    std::promise<void> waiting;

    InSequence sequence;
    EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(kNeverExpires), _, _))
        .WillOnce(Invoke([&waiting](auto path, auto state, auto timeout, auto interval, const cpp::stop_token& token) {
            waiting.set_value();
            return blockUntilStopped(path, state, timeout, interval, token);
        }));
    EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(0ms), _, _)).WillOnce(Return(osal::OsalReturnType::kFail));

    file_ready_condition.prepare();
    auto future = std::async(std::launch::async, [&file_ready_condition] {
        return file_ready_condition.wait(cpp::stop_token{}, ProcessHandle{42, nullptr}, kNeverExpires);
    });
    waiting.get_future().wait();
    file_ready_condition.handleTermination(ProcessHandle{42, nullptr}, 1);

    const Result<void> result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(FileReadyConditionTest, StopRequestDoesNotCheckFileAgain)
{
    RecordProperty(
        "Description",
        "Given an already stopped stop token. "
        "When wait is called. "
        "Then the file wait fails, the file is not checked again and wait returns kGeneralError.");

    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms);
    cpp::stop_source stop_source;
    static_cast<void>(stop_source.request_stop());

    EXPECT_CALL(file_waiter, waitForFile(_, _, _, _, _)).WillOnce(Invoke(blockUntilStopped));

    file_ready_condition.prepare();
    const Result<void> result =
        file_ready_condition.wait(stop_source.get_token(), ProcessHandle{42, nullptr}, kNeverExpires);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(FileReadyConditionTest, PrepareDiscardsTerminationOfPreviousActivation)
{
    RecordProperty(
        "Description",
        "Given a termination reported for a previous activation. "
        "When prepare is called for the next activation and wait is called. "
        "Then the stale termination does not end the wait.");

    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms);

    file_ready_condition.prepare();
    file_ready_condition.handleTermination(ProcessHandle{42, nullptr}, 0);
    file_ready_condition.prepare();

    EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(10ms), _, _))
        .WillOnce(Invoke([](auto, auto, auto, auto, const cpp::stop_token& token) {
            EXPECT_FALSE(token.stop_requested());
            return osal::OsalReturnType::kTimeout;
        }));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, ProcessHandle{42, nullptr}, 10ms);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kTimeout);
}

}  // namespace
}  // namespace score::mw::lifecycle::internal
