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

#include "tests/utils/fixtures/activation.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

// Given a correct configuration with:
//   - An initial Run Target named "Startup" containing component named
//   "control_client_test_driver"
//   - A Run Target named "run_target_app_does_report_krunning_in_time"
//   containing "control_client_test_driver" and
//   "component_does_report_krunning_in_time"
//   - A Run Target named "run_target_app_does_not_report_krunning_in_time"
//   containing "control_client_test_driver" and
//   "component_does_not_report_krunning_in_time"

using namespace score::mw::lifecycle;

using RecoveryActionSimpleRepFailure = ActivationFixture;

TEST_F(RecoveryActionSimpleRepFailure, ControlClientTestDriver)
{
    ASSERT_TRUE(check_clean({fallback_file}));

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

    TEST_STEP("Activate Run Target run_target_app_does_report_krunning_in_time")
    {
        const auto result = client->activate_run_target("run_target_app_does_report_krunning_in_time", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Callback for Run Target run_target_app_does_report_krunning_in_time")
    {
        WaitForActivation(
            RunTargetActivationSource::kStateManagerRequest, "run_target_app_does_report_krunning_in_time");
    }

    TEST_STEP("Verify fallback Run Target has not been activated")
    {
        EXPECT_FALSE(std::filesystem::exists(fallback_file)) << "Fallback Run Target should have not been activated";
    }

    TEST_STEP("Activate Run Target run_target_app_does_not_report_krunning_in_time")
    {
        const auto result = client->activate_run_target("run_target_app_does_not_report_krunning_in_time", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Callback for Run Target fallback")
    {
        WaitForActivation(RunTargetActivationSource::kRecoveryAction, "fallback_run_target");
    }

    TEST_STEP("Verify fallback Run Target was activated")
    {
        EXPECT_TRUE(std::filesystem::exists(fallback_file)) << "Fallback Run Target should have been activated";
    }
}

int main()
{
    TestRunner::Init();
    const auto result = TestRunner::RunTests();
    TestRunner::EndTest();
    return result;
}
