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
#include <gtest/gtest.h>

#include <unistd.h>
#include <limits.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>
#include <thread>

#include "score/mw/launch_manager/osal/wait_for_file.hpp"

using score::mw::lifecycle::internal::configuration::FileExistenceState;
using score::mw::lifecycle::internal::osal::OsalReturnType;
using score::mw::lifecycle::internal::osal::wait_for_file;

namespace
{

constexpr std::chrono::milliseconds kPollInterval{1U};

/// Long enough that the tests below never hit it unintentionally; the timeout itself is covered by dedicated tests.
constexpr std::chrono::milliseconds kWaitTimeout{60000U};

/// Used where the condition can never be satisfied, so that the test does not wait for kWaitTimeout.
constexpr std::chrono::milliseconds kNoWait{0U};

constexpr auto kExists = FileExistenceState::Exists;
constexpr auto kNotExisting = FileExistenceState::NotExisting;

class WaitForFileTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");

        path_ = std::string{::testing::TempDir()} + "wait_for_file_UT_" + std::to_string(getpid());
        static_cast<void>(std::remove(path_.c_str()));
    }

    void TearDown() override
    {
        static_cast<void>(std::remove(path_.c_str()));
    }

    void createFile() const
    {
        std::ofstream file{path_};
        ASSERT_TRUE(file.is_open());
    }

    void removeFile() const
    {
        ASSERT_EQ(std::remove(path_.c_str()), 0);
    }

    std::string path_{};
};

TEST_F(WaitForFileTest, ExistingFileIsReportedImmediately)
{
    RecordProperty("Description", "Verify that a file which already exists is detected without waiting.");

    createFile();

    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(wait_for_file(path_, kExists, kWaitTimeout, kPollInterval), OsalReturnType::kSuccess);
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::milliseconds{1000U});
}

TEST_F(WaitForFileTest, MissingParentDirectoryTimesOut)
{
    RecordProperty("Description", "Verify that a path below a non existing directory is treated as not yet created.");

    const std::string path = path_ + "/no_such_directory/file";

    EXPECT_EQ(wait_for_file(path, kExists, kNoWait, kPollInterval), OsalReturnType::kTimeout);
}

TEST_F(WaitForFileTest, FileCreatedWhileWaitingIsDetected)
{
    RecordProperty("Description", "Verify that a file created by another thread during the wait is detected.");

    std::thread creator{[this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds{20U});
        createFile();
    }};

    EXPECT_EQ(wait_for_file(path_, kExists, kWaitTimeout, kPollInterval), OsalReturnType::kSuccess);
    creator.join();
}

TEST_F(WaitForFileTest, DirectoryCountsAsExisting)
{
    RecordProperty("Description", "Verify that the check is not restricted to regular files.");

    EXPECT_EQ(wait_for_file(::testing::TempDir(), kExists, kWaitTimeout, kPollInterval), OsalReturnType::kSuccess);
}

TEST_F(WaitForFileTest, AbsentFileIsReportedImmediatelyForNotExisting)
{
    RecordProperty("Description", "Verify that a file which is already absent satisfies kNotExisting without waiting.");

    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(wait_for_file(path_, kNotExisting, kWaitTimeout, kPollInterval), OsalReturnType::kSuccess);
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::milliseconds{1000U});
}

TEST_F(WaitForFileTest, FileRemovedWhileWaitingIsDetected)
{
    RecordProperty(
        "Description", "Verify that a file removed by another thread during the wait satisfies kNotExisting.");

    createFile();

    std::thread remover{[this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds{20U});
        removeFile();
    }};

    EXPECT_EQ(wait_for_file(path_, kNotExisting, kWaitTimeout, kPollInterval), OsalReturnType::kSuccess);
    remover.join();
}

TEST_F(WaitForFileTest, ExistingFileTimesOutForNotExisting)
{
    RecordProperty("Description", "Verify that a file which never disappears returns kTimeout for kNotExisting.");

    createFile();
    constexpr std::chrono::milliseconds timeout{50U};

    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(wait_for_file(path_, kNotExisting, timeout, kPollInterval), OsalReturnType::kTimeout);
    EXPECT_GE(std::chrono::steady_clock::now() - start, timeout);
}

TEST_F(WaitForFileTest, MissingParentDirectoryCountsAsNotExisting)
{
    RecordProperty("Description", "Verify that a path below a non existing directory satisfies kNotExisting.");

    const std::string path = path_ + "/no_such_directory/file";

    EXPECT_EQ(wait_for_file(path, kNotExisting, kWaitTimeout, kPollInterval), OsalReturnType::kSuccess);
}

TEST_F(WaitForFileTest, PathBelowARegularFileCountsAsNotExisting)
{
    RecordProperty(
        "Description", "Verify that ENOTDIR, raised by a regular file used as a directory, is not an error.");

    createFile();
    const std::string path = path_ + "/file";

    EXPECT_EQ(wait_for_file(path, kNotExisting, kWaitTimeout, kPollInterval), OsalReturnType::kSuccess);
}

TEST_F(WaitForFileTest, PathBelowARegularFileTimesOutForExists)
{
    RecordProperty("Description", "Verify that ENOTDIR is treated as not yet created while waiting for kExists.");

    createFile();
    const std::string path = path_ + "/file";

    EXPECT_EQ(wait_for_file(path, kExists, kNoWait, kPollInterval), OsalReturnType::kTimeout);
}

TEST_F(WaitForFileTest, EmptyPathFailsForNotExisting)
{
    RecordProperty("Description", "Verify that an empty path is rejected for kNotExisting instead of being polled.");

    EXPECT_EQ(wait_for_file(std::string_view{}, kNotExisting, kWaitTimeout, kPollInterval), OsalReturnType::kFail);
}

TEST_F(WaitForFileTest, MissingFileReturnsTimeoutWhenTheTimeoutElapses)
{
    RecordProperty("Description", "Verify that the wait ends with kTimeout once the given timeout has elapsed.");

    constexpr std::chrono::milliseconds timeout{50U};

    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(wait_for_file(path_, kExists, timeout, kPollInterval), OsalReturnType::kTimeout);
    EXPECT_GE(std::chrono::steady_clock::now() - start, timeout);
}

TEST_F(WaitForFileTest, ZeroTimeoutStillChecksOnce)
{
    RecordProperty("Description", "Verify that an existing file is detected even with a zero timeout.");

    createFile();

    EXPECT_EQ(wait_for_file(path_, kExists, kNoWait, kPollInterval), OsalReturnType::kSuccess);
}

TEST_F(WaitForFileTest, ZeroTimeoutOnMissingFileReturnsTimeout)
{
    RecordProperty("Description", "Verify that a zero timeout does not wait for a file which does not exist yet.");

    EXPECT_EQ(wait_for_file(path_, kExists, kNoWait, kPollInterval), OsalReturnType::kTimeout);
}

TEST_F(WaitForFileTest, PollIntervalDoesNotExtendTheTimeout)
{
    RecordProperty("Description", "Verify that a poll interval longer than the timeout does not delay the kTimeout.");

    constexpr std::chrono::milliseconds timeout{20U};
    constexpr std::chrono::milliseconds poll_interval{5000U};

    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(wait_for_file(path_, kExists, timeout, poll_interval), OsalReturnType::kTimeout);
    EXPECT_LT(std::chrono::steady_clock::now() - start, poll_interval);
}

TEST_F(WaitForFileTest, EmptyPathFails)
{
    RecordProperty("Description", "Verify that an empty path is rejected instead of being polled.");

    EXPECT_EQ(wait_for_file(std::string_view{}, kExists, kWaitTimeout, kPollInterval), OsalReturnType::kFail);
}

TEST_F(WaitForFileTest, NonNullTerminatedPathFails)
{
    RecordProperty("Description", "Verify that a path which is not null terminated is rejected instead of polled.");

    createFile();
    const std::string path = path_ + "x";
    const std::string_view not_terminated{path.data(), path.size() - 1U};

    EXPECT_EQ(wait_for_file(not_terminated, kExists, kWaitTimeout, kPollInterval), OsalReturnType::kFail);
}

TEST_F(WaitForFileTest, TooLongPathFails)
{
    RecordProperty("Description", "Verify that a path which does not fit into PATH_MAX is rejected.");

    const std::string path(PATH_MAX + 1U, 'a');
    EXPECT_EQ(wait_for_file(path, kExists, kWaitTimeout, kPollInterval), OsalReturnType::kFail);
}

}  // namespace
