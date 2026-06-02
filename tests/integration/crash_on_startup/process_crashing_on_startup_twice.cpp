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
#include <filesystem>
#include <iostream>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/lifecycle_client.h>

TEST(CrashOnStartup, ProcessCrashingOnStartupTwice)
{
    TEST_STEP("Report kRunning")
    {
        auto result = score::mw::lifecycle::LifecycleClient{}.ReportExecutionState(score::mw::lifecycle::ExecutionState::kRunning);
        ASSERT_TRUE(result.has_value()) << "ReportExecutionState() failed: " << result.error().Message();
    }
}

void deploy_and_crash_if_not_present(const std::string_view name)
{
    if (!std::filesystem::exists(name))
    {
        std::cout << "Process crashing on startup..." << std::endl;
        if (!touch_file(name))
        {
            std::cout << "Failed to deploy marker file!" << std::endl;
        }
        std::abort();
    }
}

int main()
{
    deploy_and_crash_if_not_present(crashed_once_file);
    deploy_and_crash_if_not_present(crashed_twice_file);

    std::cout << "Process starting successfully..." << std::endl;
    return TestRunner(__FILE__).RunTests();
}
