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

/// @brief Configures a ready condition whose every termination ends the wait.
constexpr bool kNotSelfTerminating = false;

/// @brief Configures a ready condition whose successful exit does not end the wait.
constexpr bool kSelfTerminating = true;

TEST(FileReadyConditionTest, WaitCallsWaitForFile)
{
    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kNotSelfTerminating);

    EXPECT_CALL(
        file_waiter, waitForFile(Eq(kFilePath), Eq(configuration::FileExistenceState::Exists), Eq(10ms), Eq(5ms), _))
        .WillOnce(Return(osal::OsalReturnType::kSuccess));

    static_cast<void>(file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{}, 10ms));
}

TEST(FileReadyConditionTest, WaitSucceedsReturnsSuccess)
{
    NiceMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kNotSelfTerminating);

    ON_CALL(file_waiter, waitForFile(_, _, _, _, _)).WillByDefault(Return(osal::OsalReturnType::kSuccess));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{}, 10ms);

    EXPECT_TRUE(result.has_value());
}

TEST(FileReadyConditionTest, WaitFailsReturnsError)
{
    NiceMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kNotSelfTerminating);

    ON_CALL(file_waiter, waitForFile(_, _, _, _, _)).WillByDefault(Return(osal::OsalReturnType::kFail));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{}, 10ms);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(FileReadyConditionTest, WaitTimesOutReturnsTimeout)
{
    NiceMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kNotSelfTerminating);

    ON_CALL(file_waiter, waitForFile(_, _, _, _, _)).WillByDefault(Return(osal::OsalReturnType::kTimeout));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, EmptyHandle{}, 10ms);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kTimeout);
}

TEST(FileReadyConditionTest, WaitAcceptsVariousHandles)
{
    NiceMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kNotSelfTerminating);

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
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kNotSelfTerminating);

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
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kNotSelfTerminating);

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
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kNotSelfTerminating);
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

TEST(FileReadyConditionTest, StopRequestedBeforeWaitSucceedsIfFileReachedState)
{
    RecordProperty(
        "Description",
        "Given an already stopped stop token. "
        "When wait is called and the file has already reached its state. "
        "Then the file is checked once more without waiting and wait returns success, since a stop only "
        "interrupts the wait.");

    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kNotSelfTerminating);
    cpp::stop_source stop_source;
    static_cast<void>(stop_source.request_stop());

    {
        const InSequence in_sequence;
        EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(kNeverExpires), _, _)).WillOnce(Invoke(blockUntilStopped));
        EXPECT_CALL(
            file_waiter, waitForFile(Eq(kFilePath), Eq(configuration::FileExistenceState::Exists), Eq(0ms), _, _))
            .WillOnce(Invoke([](auto, auto, auto, auto, const cpp::stop_token& token) {
                EXPECT_FALSE(token.stop_possible());
                return osal::OsalReturnType::kSuccess;
            }));
    }

    file_ready_condition.prepare();
    const Result<void> result =
        file_ready_condition.wait(stop_source.get_token(), ProcessHandle{42, nullptr}, kNeverExpires);

    EXPECT_TRUE(result.has_value());
}

TEST(FileReadyConditionTest, StopRequestedBeforeWaitFailsIfFileDidNotReachState)
{
    RecordProperty(
        "Description",
        "Given an already stopped stop token. "
        "When wait is called and the file has not reached its state. "
        "Then the final check fails and wait returns kGeneralError, not kTimeout.");

    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kNotSelfTerminating);
    cpp::stop_source stop_source;
    static_cast<void>(stop_source.request_stop());

    {
        const InSequence in_sequence;
        EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(kNeverExpires), _, _)).WillOnce(Invoke(blockUntilStopped));
        EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(0ms), _, _)).WillOnce(Return(osal::OsalReturnType::kTimeout));
    }

    file_ready_condition.prepare();
    const Result<void> result =
        file_ready_condition.wait(stop_source.get_token(), ProcessHandle{42, nullptr}, kNeverExpires);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

TEST(FileReadyConditionTest, StopRequestedDuringWaitSucceedsIfFileReachedState)
{
    RecordProperty(
        "Description",
        "Given a wait for a file. "
        "When a stop is requested while waiting and the file has reached its state by then. "
        "Then the final check succeeds and wait returns success.");

    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kNotSelfTerminating);
    cpp::stop_source stop_source;

    {
        const InSequence in_sequence;
        EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(kNeverExpires), _, _))
            .WillOnce(Invoke([&stop_source](auto, auto, auto, auto, const cpp::stop_token& token) {
                static_cast<void>(stop_source.request_stop());
                // The stop request reaches the file wait.
                EXPECT_TRUE(token.stop_requested());
                return osal::OsalReturnType::kFail;
            }));
        EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(0ms), _, _)).WillOnce(Return(osal::OsalReturnType::kSuccess));
    }

    file_ready_condition.prepare();
    const Result<void> result =
        file_ready_condition.wait(stop_source.get_token(), ProcessHandle{42, nullptr}, kNeverExpires);

    EXPECT_TRUE(result.has_value());
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
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kNotSelfTerminating);

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

TEST(FileReadyConditionTest, SelfTerminatingExitZeroBeforeWaitDoesNotEndWait)
{
    RecordProperty(
        "Description",
        "Given a self-terminating process which exited with status 0 before wait is called. "
        "When wait is called. "
        "Then the file is waited for with the complete timeout and a token which is not stopped, and wait "
        "succeeds once the file has reached its state.");

    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kSelfTerminating);

    file_ready_condition.prepare();
    file_ready_condition.handleTermination(ProcessHandle{42, nullptr}, 0);

    EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(kNeverExpires), _, Property(&cpp::stop_token::stop_requested, false)))
        .WillOnce(Return(osal::OsalReturnType::kSuccess));

    EXPECT_TRUE(file_ready_condition.wait(cpp::stop_token{}, ProcessHandle{42, nullptr}, kNeverExpires).has_value());
}

TEST(FileReadyConditionTest, SelfTerminatingExitZeroDuringWaitDoesNotEndWait)
{
    RecordProperty(
        "Description",
        "Given a self-terminating process whose file is waited for. "
        "When the process exits with status 0 during the wait. "
        "Then the wait is not interrupted and succeeds once the file has reached its state.");

    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kSelfTerminating);

    EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(kNeverExpires), _, _))
        .WillOnce(Invoke([&file_ready_condition](auto, auto, auto, auto, const cpp::stop_token& token) {
            file_ready_condition.handleTermination(ProcessHandle{42, nullptr}, 0);
            EXPECT_FALSE(token.stop_requested());
            return osal::OsalReturnType::kSuccess;
        }));

    file_ready_condition.prepare();
    EXPECT_TRUE(file_ready_condition.wait(cpp::stop_token{}, ProcessHandle{42, nullptr}, kNeverExpires).has_value());
}

TEST(FileReadyConditionTest, SelfTerminatingExitZeroTimesOutIfFileDidNotReachState)
{
    RecordProperty(
        "Description",
        "Given a self-terminating process which exited with status 0. "
        "When the file does not reach its state within the timeout. "
        "Then wait returns kTimeout, since the exit did not end the wait.");

    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kSelfTerminating);

    file_ready_condition.prepare();
    file_ready_condition.handleTermination(ProcessHandle{42, nullptr}, 0);

    // The strict mock fails the test if the file is checked once more, as after an interrupted wait.
    EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(10ms), _, _)).WillOnce(Return(osal::OsalReturnType::kTimeout));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, ProcessHandle{42, nullptr}, 10ms);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kTimeout);
}

TEST(FileReadyConditionTest, SelfTerminatingNonZeroExitEndsWait)
{
    RecordProperty(
        "Description",
        "Given a self-terminating process which exited with a non-zero status before wait is called. "
        "When wait is called and the file has not reached its state. "
        "Then the wait ends without waiting for the timeout and returns kGeneralError.");

    StrictMock<osal::MockIFileWaiter> file_waiter;
    const FileReadyCondition file_ready_condition(
        file_waiter, kFilePath, configuration::FileExistenceState::Exists, 5ms, kSelfTerminating);

    file_ready_condition.prepare();
    file_ready_condition.handleTermination(ProcessHandle{42, nullptr}, 1);

    InSequence sequence;
    EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(kNeverExpires), _, _)).WillOnce(Invoke(blockUntilStopped));
    EXPECT_CALL(file_waiter, waitForFile(_, _, Eq(0ms), _, _)).WillOnce(Return(osal::OsalReturnType::kTimeout));

    const Result<void> result = file_ready_condition.wait(cpp::stop_token{}, ProcessHandle{42, nullptr}, kNeverExpires);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ExecErrc::kGeneralError);
}

}  // namespace
}  // namespace score::mw::lifecycle::internal
