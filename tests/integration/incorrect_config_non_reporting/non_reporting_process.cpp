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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <csignal>

#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/lifecycle_client.h>

TEST(NonReporting, Process)
{
    using ipc = score::lcm::internal::osal::IpcCommsSync;

    // Map the sync_fd to some shared memory of size 0. With sync_fd=3, this is likely to happen by default
    // As there is no comms channel, using sync_fd in this process should not cause a crash.
    auto fd = shm_open("some_shared_memory", O_CREAT | O_RDWR, 0);
    ASSERT_NE(fd, -1) << "Failed to open shared memory: " << strerror(errno);
    shm_unlink("some_shared_memory");
    struct stat res;
    if (fstat(ipc::sync_fd, &res) != 0)
    {  // sync_fd is closed
        auto dup_res = dup2(ipc::sync_fd, fd);
        ASSERT_NE(dup_res, -1) << "dup2 failed: " << strerror(errno);
    }

    // Invalid kRunning report
    auto result = score::mw::lifecycle::LifecycleClient{}.ReportExecutionState(score::mw::lifecycle::ExecutionState::kRunning);
    EXPECT_FALSE(result.has_value()) << "ReportExecutionState() should not succeed";

    close(ipc::sync_fd);
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd).RunTests();
}
