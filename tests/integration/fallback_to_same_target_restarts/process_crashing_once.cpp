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

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

TEST(FallbackToSameTargetRestarts, CrashingProcess)
{
    const std::string_view crash_file = "process_crashed";

    if (std::filesystem::exists(crash_file))
    {
        TEST_STEP("Create process file")
        {
            ASSERT_TRUE(touch_file("process_started_normally"));
        }

        // Don't report running until the file is created. This prevents the
        // test driver exiting too early and causing the test to fail.
        TEST_STEP("Report running")
        {
            score::mw::lifecycle::report_running();
        }
    }
    else
    {
        TEST_STEP("Report running")
        {
            score::mw::lifecycle::report_running();
        }

        // Limitation: we can't wait for run target activation to complete
        sleep(1);

        TEST_STEP("Crash")
        {
            std::cout << "Process crashing..." << std::endl;
            if (!touch_file(crash_file))
            {
                std::cout << "Failed to deploy marker file!" << std::endl;
            }
            exit(1);
        }
    }
}

int main()
{
    TestRunner(__FILE__, TerminationBehavior::kContinue).RunTests();
}
