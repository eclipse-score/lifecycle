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

#include "tests/utils/fixtures/activation.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

using CrashOnStartup = ActivationFixture;

TEST_F(CrashOnStartup, ControlClientTestDriver)
{
    ASSERT_TRUE(check_clean({crashCountPath(1), crashCountPath(2), crashCountPath(3), fallback_file}));

    std::unique_ptr<ILmControl> client;

    TEST_STEP("Initialize client")
    {
        auto maybe_client = InitializeClient();
        ASSERT_TRUE(maybe_client.has_value()) << maybe_client.error().Message();
        client = std::move(maybe_client).value();
    }

    TEST_STEP("Report running")
    {
        report_running();
    }

    TEST_STEP("Callback for Run Target Startup")
    {
        WaitForActivation(RunTargetActivationSource::kInitialActivation, "Startup");
    }

    // Given a process that crashes on startup n times, but is configured to retry n times - so it eventually
    // succeeds. The behaviour is identical for the different crash counts, so it is parameterized over the
    // corresponding Run Targets. Each Run Target's process persists its crash count in its own file, so the
    // Run Targets do not interfere with each other.
    for (const std::string_view run_target :
         {"run_target_crash_on_startup_two_times", "run_target_crash_on_startup_three_times"})
    {
        TEST_STEP(std::string{"Launch "} + std::string{run_target})
        {
            const auto result = client->activate_run_target(score::mw::lifecycle::RunTargetName{run_target}, true);
            EXPECT_TRUE(result.has_value()) << result.error().Message();
        }

        // Then, the LM should restart it and eventually succeed
        TEST_STEP(std::string{"Callback for Run Target "} + std::string{run_target})
        {
            WaitForActivation(RunTargetActivationSource::kStateManagerRequest, RunTargetName{run_target});
        }

        TEST_STEP("Verify fallback Run Target was not activated, i.e. process eventually started successfully")
        {
            EXPECT_FALSE(std::filesystem::exists(fallback_file)) << "Fallback Run Target should not be activated yet";
        }
    }

    // Given a process that crashes on startup but is not allowed to retry (number_of_attempts=0)
    TEST_STEP("Attempt to launch process crashing on startup without retries")
    {
        const auto result = client->activate_run_target("run_target_crash_on_startup_once_but_no_retries", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Callback for Run Target fallback")
    {
        WaitForActivation(RunTargetActivationSource::kRecoveryAction, "fallback_run_target");
    }

    TEST_STEP("Verify fallback Run Target was activated")
    {
        // This verifies that a fallback process was actually started - the launch manager
        // did not just send an event without taking the action.
        EXPECT_TRUE(std::filesystem::exists(fallback_file)) << "Fallback Run Target should have been activated";
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
