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

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{
std::string component_name()
{
    const char* process_id = std::getenv("PROCESSIDENTIFIER");
    return process_id ? std::string{process_id} : std::string{"gated_process"};
}
}  // namespace

// Reports running only once the control client creates the release file.
TEST(RunTargetRequestHandling, GatedProcess)
{
    const std::string name = component_name();

    TEST_STEP("Signal start")
    {
        EXPECT_TRUE(touch_file(started_file(name)));
    }

    TEST_STEP("Wait for release")
    {
        while (!TestRunner::exitRequested && !std::filesystem::exists(release_file(name)))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    if (TestRunner::exitRequested)
    {
        return;
    }

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
        EXPECT_TRUE(touch_file(running_file(name)));
    }

    while (!TestRunner::exitRequested)
    {
        pause();
    }
}

int main()
{
    // One XML result per deployed component.
    return TestRunner(component_name()).RunTests();
}
