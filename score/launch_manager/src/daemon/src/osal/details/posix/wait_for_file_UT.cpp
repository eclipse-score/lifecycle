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

using score::mw::lifecycle::internal::osal::OsalReturnType;
using score::mw::lifecycle::internal::osal::wait_for_file;

namespace
{

constexpr std::chrono::milliseconds kPollInterval{1U};

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

    std::string path_{};
};

TEST_F(WaitForFileTest, ExistingFileIsReportedImmediately)
{
    RecordProperty("Description", "Verify that a file which already exists is detected without waiting.");

    createFile();

    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(wait_for_file(std::chrono::milliseconds{5000U}, path_, kPollInterval), OsalReturnType::kSuccess);
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::milliseconds{1000U});
}

TEST_F(WaitForFileTest, ZeroTimeoutStillChecksOnce)
{
    RecordProperty("Description", "Verify that a timeout of zero performs a single existence check.");

    createFile();

    EXPECT_EQ(wait_for_file(std::chrono::milliseconds{0U}, path_, kPollInterval), OsalReturnType::kSuccess);
}

TEST_F(WaitForFileTest, MissingFileTimesOut)
{
    RecordProperty(
        "Description", "Verify that waiting for a file which never appears returns kTimeout after the given duration.");

    const auto timeout = std::chrono::milliseconds{50U};
    const auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(wait_for_file(timeout, path_, kPollInterval), OsalReturnType::kTimeout);
    EXPECT_GE(std::chrono::steady_clock::now() - start, timeout);
}

TEST_F(WaitForFileTest, MissingParentDirectoryTimesOut)
{
    RecordProperty("Description", "Verify that a path below a non existing directory is treated as not yet created.");

    const std::string path = path_ + "/no_such_directory/file";
    EXPECT_EQ(wait_for_file(std::chrono::milliseconds{10U}, path, kPollInterval), OsalReturnType::kTimeout);
}

TEST_F(WaitForFileTest, FileCreatedWhileWaitingIsDetected)
{
    RecordProperty("Description", "Verify that a file created by another thread during the wait is detected.");

    std::thread creator{[this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds{20U});
        createFile();
    }};

    EXPECT_EQ(wait_for_file(std::chrono::milliseconds{5000U}, path_, kPollInterval), OsalReturnType::kSuccess);
    creator.join();
}

TEST_F(WaitForFileTest, DirectoryCountsAsExisting)
{
    RecordProperty("Description", "Verify that the check is not restricted to regular files.");

    EXPECT_EQ(
        wait_for_file(std::chrono::milliseconds{0U}, ::testing::TempDir(), kPollInterval), OsalReturnType::kSuccess);
}

TEST_F(WaitForFileTest, EmptyPathFails)
{
    RecordProperty("Description", "Verify that an empty path is rejected instead of being polled.");

    EXPECT_EQ(
        wait_for_file(std::chrono::milliseconds{5000U}, std::string_view{}, kPollInterval), OsalReturnType::kFail);
}

TEST_F(WaitForFileTest, TooLongPathFails)
{
    RecordProperty("Description", "Verify that a path which does not fit into PATH_MAX is rejected.");

    const std::string path(PATH_MAX + 1U, 'a');
    EXPECT_EQ(wait_for_file(std::chrono::milliseconds{5000U}, path, kPollInterval), OsalReturnType::kFail);
}

}  // namespace
