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
#include "tests/utils/test_helper/test_runner.hpp"
#include <fcntl.h>
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>
#include <sys/types.h>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <thread>

using namespace score::mw::lifecycle;

// Given a correct configuration with:
//   - An initial Run Target named "Startup" containing "control_client_test_driver"
//   - A Run Target named "run_target_crashing_app_on_runtime" containing "control_client_test_driver" and
//     "component_crashing_once"
//   - A Run Target named "supervision_failure" containing "control_client_test_driver" and "component_supervised"
//   - A fallback Run Target containing all of the above components
//
// Verifies that a process is restarted in the fallback Run Target after
//   1. it crashed, and
//   2. it failed alive supervision.

namespace
{
/// @return The pid stored in `pidfile`, or -1 if it could not be read.
pid_t read_pid(const std::string& pidfile)
{
    std::ifstream file{pidfile};
    pid_t pid{-1};
    if (!(file >> pid))
    {
        return -1;
    }
    return pid;
}
}  // namespace

TEST(FallbackToSameTargetRestarts, ControlClientTestDriver)
{
    const std::string_view process_file = "process_started_normally";
    const std::string pidfile = "supervised_app.pid";

    ASSERT_TRUE(check_clean({process_file, pidfile}));

    std::unique_ptr<ILmControl> client;

    TEST_STEP("Create client")
    {
        auto client_result = ILmControl::Create("StateManager/LaunchManager/Instance");
        ASSERT_TRUE(client_result.has_value()) << client_result.error().Message();
        client = std::move(client_result).value();
    }

    TEST_STEP("Register callback")
    {
        const auto result = client->register_run_target_activation_callback(push_event);
        ASSERT_TRUE(result.has_value());
    }

    TEST_STEP("Report running")
    {
        report_running();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target Startup")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kInitialActivation);
            EXPECT_EQ(target, "Startup");
        }
    });

    TEST_STEP("Start crashing process")
    {
        const auto result = client->activate_run_target("run_target_crashing_app_on_runtime", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target run_target_crashing_app_on_runtime")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "run_target_crashing_app_on_runtime");
        }
    });

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target fallback")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kRecoveryAction);
            EXPECT_EQ(target, "fallback_run_target");
        }
    });

    TEST_STEP("Verify process was restarted")
    {
        EXPECT_TRUE(std::filesystem::exists(process_file)) << "Process did not restart successfully";
    }

    TEST_STEP("Activate Run Target supervision_failure")
    {
        const auto result = client->activate_run_target("supervision_failure", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target supervision_failure")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "supervision_failure");
        }
    });

    pid_t first_pid{-1};
    TEST_STEP("Read pid of supervised process")
    {
        first_pid = read_pid(pidfile);
        ASSERT_GT(first_pid, 0) << "Could not read pid from " << pidfile;
    }

    TEST_STEP("Make alive supervision fail")
    {
        // The supervised process stops reporting alive notifications when it receives SIGUSR1.
        EXPECT_EQ(kill(first_pid, SIGUSR1), 0);
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target fallback after supervision failure")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kRecoveryAction);
            EXPECT_EQ(target, "fallback_run_target");
        }
    });

    TEST_STEP("Verify supervised process was restarted")
    {
        const pid_t second_pid = read_pid(pidfile);
        ASSERT_GT(second_pid, 0) << "Could not read pid from " << pidfile;
        EXPECT_NE(second_pid, first_pid) << "Supervised process was not restarted";
    }
}

int main()
{
    TestRunner::Init();
    const auto result = TestRunner::RunTests();
    TestRunner::EndTest();
    TestRunner::WaitForTermination();
    return result;
}
