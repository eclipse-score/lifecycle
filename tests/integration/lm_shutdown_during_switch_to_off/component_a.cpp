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
#include <thread>

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{
/// @brief How long component_a stalls while it is being terminated. By stalling here, component_a keeps the switch (to
/// the "Off" run target) in the STOP phase, giving the test a deterministic window to send SIGTERM to the launch
/// manager while the switch to Off is still in progress.
///
/// It must be comfortably larger than the time the test needs to observe
/// `a_terminating` and deliver the SIGTERM to the launch manager, so the switch
/// to Off is still in progress when that SIGTERM arrives. It must also be
/// comfortably smaller than component_a's configured shutdown_timeout: the launch
/// manager honours that per-process shutdown_timeout during its own shutdown, so
/// component_a is given time to exit on its own and terminates gracefully (and
/// writes its XML result) rather than being force-terminated (SIGKILLed).
constexpr unsigned int kTerminationDelaySeconds = 2U;
}  // namespace

TEST(LmShutdownDuringSwitchToOff, ComponentA)
{
    const auto pid = getpid();
    const std::string step_msg = "Report running with pid == " + std::to_string(pid);

    TEST_STEP(step_msg)
    {
        EXPECT_TRUE(touch_file(a_started)) << "failed to deploy file";
        score::mw::lifecycle::report_running();
    }

    // Wait until the launch manager asks us to terminate (SIGTERM), which happens
    // when the switch away from run_target_a (to the "Off" run target) begins.
    // Poll the flag rather than pause(): a process-directed SIGTERM may be handled
    // on a background thread (e.g. alive reporting), which would set exitRequested
    // without waking a main thread blocked in pause().
    while (!TestRunner::exitRequested)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
