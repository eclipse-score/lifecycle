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

#include "tests/utils/test_helper/gated_process.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{
/// @return The component this binary is deployed as (PROCESSIDENTIFIER).
std::string component_name()
{
    const char* process_id = std::getenv("PROCESSIDENTIFIER");
    return process_id ? std::string{process_id} : std::string{"gated_process"};
}
}  // namespace

// Creates gatedStartedPath() on start, then reports running only once the test
// creates gatedReleasePath(). This keeps the activation of its Run Target in
// progress for as long as the test needs. If the launch manager stops it before it
// is released, it exits without reporting running.
TEST(GatedProcess, ReportsRunningOnRelease)
{
    const std::string name = component_name();

    TEST_STEP("Signal start")
    {
        EXPECT_TRUE(touch_file(gatedStartedPath(name)));
    }

    TEST_STEP("Wait for release")
    {
        while (!TestRunner::exitRequested && !std::filesystem::exists(gatedReleasePath(name)))
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
    }

    while (!TestRunner::exitRequested)
    {
        pause();
    }
}

int main()
{
    // Name the XML result after the deployed component so multiple deployments of this
    // shared binary don't collide.
    return TestRunner(component_name()).RunTests();
}
