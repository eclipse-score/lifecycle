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
#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/lcm/lifecycle_client.h>

int g_argc;
char** g_argv;

TEST(RecoveryActionSimpleRepFailure, RecoveryActionReporting)
{
    // Check arguments
    TEST_STEP("Check args")
    {
        ASSERT_GT(g_argc, 1) << "Wrong number of arguments";
        ASSERT_FALSE((g_argv[1][0] != '0') && (atoi(g_argv[1])) == 0) << "Argument must be a number";
    }
    // Report kRunning with the appropriate delay
    TEST_STEP("Report kRunning from RecoveryActionReporting")
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(atoi(g_argv[1])));

        auto result = score::lcm::LifecycleClient{}.ReportExecutionState(score::lcm::ExecutionState::kRunning);
        EXPECT_TRUE(result.has_value()) << "ReportExecutionState() failed: " << result.error().Message();
    }
}

int main(int argc, char** argv)
{
    g_argc = argc;
    g_argv = argv;

    return TestRunner(__FILE__, false).RunTests();
}
