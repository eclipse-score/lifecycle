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

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{
/// @brief How long component_a stalls while being terminated, keeping the switch to Off
/// in progress so the test can send SIGTERM to the launch manager in time. Must be smaller
/// than component_a's configured shutdown_timeout so it still exits gracefully.
constexpr unsigned int kTerminationDelaySeconds = 2U;
}  // namespace

TEST(LmShutdownDuringSwitchToOff, ComponentA)
{
    TEST_STEP("Report running")
    {
        EXPECT_TRUE(touch_file(a_started)) << "failed to deploy file";
        score::mw::lifecycle::report_running();
    }

    // Wait until the launch manager asks us to terminate (SIGTERM), which happens
    // when the switch away from run_target_a (to the "Off" run target) begins.
    while (!TestRunner::exitRequested)
    {
        pause();
    }

    TEST_STEP("Stall during termination to keep the run-target switch in progress")
    {
        EXPECT_TRUE(touch_file(a_terminating)) << "failed to deploy file";
        static_cast<void>(sleep(kTerminationDelaySeconds));
    }
}

int main()
{
    return TestRunner(__FILE__).RunTests();
}
