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
/// @brief How long component_a stalls while it is being terminated. During a
/// run-target switch the launch manager runs the STOP phase (terminating
/// no-longer-needed processes) fully before the START phase (activating newly
/// needed processes). By stalling here, component_a keeps the switch (to the
/// "Off" run target) in the STOP phase, giving the test a deterministic window
/// to send SIGTERM to the launch manager while the switch to Off is still in
/// progress.
///
/// It must be comfortably larger than the time the test needs to observe
/// `a_terminating` and deliver the SIGTERM to the launch manager AND larger than
/// the launch manager's fixed shutdown grace period (see the NOTE in
/// ProcessGroupManager::allProcessGroupsOff): the shutdown does NOT respect the
/// per-process shutdown_timeout, so this stall outlives that grace period and
/// component_a is force-terminated (SIGKILLed) - it does not write an XML result.
/// It must also be smaller than component_a's configured shutdown_timeout so the
/// STOP job does not SIGKILL it on its own before the launch manager SIGTERM is
/// handled (which would end the switch to Off early and defeat the test).
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
