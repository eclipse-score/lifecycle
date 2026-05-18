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
#include <score/lcm/internal/osal/osalipccomms.hpp>
#include <score/lcm/lifecycle_client.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "tests/utils/test_helper/test_helper.hpp"
#include <csignal>

TestRunner runner = TestRunner(__FILE__, true);

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
    auto result = score::lcm::LifecycleClient{}.ReportExecutionState(score::lcm::ExecutionState::kRunning);
    EXPECT_FALSE(result.has_value()) << "ReportExecutionState() should not succeed";

    close(ipc::sync_fd);
}

int main()
{
    runner.exitRequested = true;
    return runner.RunTests();
}
