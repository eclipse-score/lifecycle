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
#include <cerrno>
#include <csignal>
#include <filesystem>
#include <fstream>

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

// The Launch Manager shall shut a process down by sending it a SIGTERM, and, if
// the process does not terminate itself in time, a SIGKILL.
//
// The managed shutdown_signal_process installs a SIGTERM handler that records the SIGTERM
// (writing its PID to `sigterm_received_file`) and then blocks instead of
// terminating, forcing the Launch Manager to send SIGKILL.
//
// We drive the shutdown by activating "Running" (which starts shutdown_signal_process) and
// then switching back to "Startup". "Startup" no longer depends on shutdown_signal_process,
// so it is terminated, while the control daemon itself stays alive (it is part of
// "Startup") and can therefore assert the outcome. Switching to "Off" instead
// would terminate the control daemon too, so it could not run the assertion.
TEST(ShutdownSignal, Daemon)
{
    const auto client = score::mw::lifecycle::ILmControl::Create("StateManager/LaunchManager/Instance");
    ASSERT_TRUE(client.has_value());

    ASSERT_TRUE(check_clean({test_end_location, sigterm_received_file}));

    TEST_STEP("Control daemon report running")
    {
        score::mw::lifecycle::report_running();
    }

    TEST_STEP("Activate RunTarget Running")
    {
        auto result = client->get()->activate_run_target("Running");
        EXPECT_TRUE(result.has_value()) << "Activating target Running failed: " << result.error().Message();
    }

    // Switching away from "Running" terminates shutdown_signal_process. Because it does not
    // self-terminate on SIGTERM, the Launch Manager must escalate to SIGKILL for
    // the transition to complete.
    TEST_STEP("Activate RunTarget Startup")
    {
        auto result = client->get()->activate_run_target("Startup");
        EXPECT_TRUE(result.has_value()) << "Activating target Startup failed: " << result.error().Message();
    }

    TEST_STEP("Verify SIGTERM was received and SIGKILL forced termination")
    {
        // SIGTERM was delivered: the process recorded its PID before blocking.
        ASSERT_TRUE(std::filesystem::exists(sigterm_received_file))
            << "shutdown_signal_process did not receive a SIGTERM during shutdown";

        // Read back the PID the process wrote from within its SIGTERM handler.
        pid_t pid{};
        std::ifstream pid_file{std::string{sigterm_received_file}, std::ios::binary};
        ASSERT_TRUE(pid_file.read(reinterpret_cast<char*>(&pid), sizeof(pid)))
            << "Failed to read the PID from " << sigterm_received_file;

        // SIGKILL forced termination: the process never self-terminates, so its
        // absence proves the Launch Manager escalated to SIGKILL.
        EXPECT_EQ(kill(pid, 0), -1) << "shutdown_signal_process (pid " << pid
                                    << ") is still alive; it was not SIGKILLed";
        EXPECT_EQ(errno, ESRCH) << "unexpected errno probing shutdown_signal_process (pid " << pid << ")";
    }

    TEST_STEP("Activate RunTarget Off")
    {
        const auto result = client->get()->activate_run_target("Off");
        EXPECT_TRUE(result.has_value()) << "Expected Off activation to succeed";
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
