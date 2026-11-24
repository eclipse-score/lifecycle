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

#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

// Given a correct configuration with:
//   - A default ready_timeout_ms of 10000 ms and a recovery action switching to
//     "fallback_run_target"
//   - An initial Run Target named "Startup" containing "control_client_test_driver"
//   - A Run Target named "run_target_slow_activation" with a transition_timeout_ms of 1000 ms,
//     containing "control_client_test_driver" and "slow_starting_process", which sleeps 3000 ms
//     before reporting running
//   - A fallback Run Target containing "control_client_test_driver" and "verification_component",
//     which touches `fallback_file`
TEST(RunTargetTransitionTimeout, ControlClientTestDriver)
{
    ASSERT_TRUE(check_clean({fallback_file}));

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

    TEST_STEP("Wait for client connection")
    {
        ASSERT_TRUE(client->wait_for_connection().has_value());
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

    TEST_STEP("Verify the fallback Run Target was not activated during startup")
    {
        ASSERT_FALSE(std::filesystem::exists(fallback_file))
            << "Fallback Run Target was activated before the Run Target under test was requested";
    }

    TEST_STEP("Activate Run Target run_target_slow_activation")
    {
        const auto result = client->activate_run_target("run_target_slow_activation", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    // The next activation reported back is the recovery action, not the requested Run
    // Target: the activation of "run_target_slow_activation" never completes, because
    // it outlasts the transition timeout configured for it.
    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for the fallback Run Target")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kRecoveryAction)
                << "the Run Target under test was activated instead of timing out";
            EXPECT_EQ(target, "fallback_run_target");
        }
    });

    TEST_STEP("Verify the fallback Run Target was activated")
    {
        EXPECT_TRUE(std::filesystem::exists(fallback_file))
            << "the recovery action was reported but the fallback Run Target was not started";
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
