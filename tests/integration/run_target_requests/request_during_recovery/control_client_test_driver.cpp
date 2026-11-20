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
#include "tests/utils/fixtures/activation.hpp"
#include <gtest/gtest.h>

using namespace score::mw::lifecycle;

TEST_F(ActivationFixture, NonForcedRequestDuringRecoveryAction)
{
    ASSERT_TRUE(check_clean(
        {gatedStartedPath("failing_component"),
         gatedReleasePath("failing_component"),
         gatedStartedPath("fallback_gated"),
         gatedReleasePath("fallback_gated")}));

    InitializeClient();

    TEST_STEP("Wait for Startup activation")
    {
        WaitForActivation(RunTargetActivationSource::kInitialActivation, "Startup");
    }

    TEST_STEP("Force activate run_target_with_failing_component")
    {
        const auto result = client->activate_run_target("run_target_with_failing_component", true);
        ASSERT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Wait for recovery action to begin")
    {
        // The failing_component will not report running in time (ready_timeout_ms is 200ms)
        // This will trigger a recovery action to switch to fallback_run_target
        // Wait for the activation to be in progress
        ASSERT_TRUE(wait_for_activation_in_progress(*client));
        ASSERT_TRUE(wait_for_file(gatedStartedPath("fallback_gated")));
    }

    TEST_STEP("Send non-forced request for queued_run_target during recovery action")
    {
        // This request should be queued, not rejected
        const auto result = client->activate_run_target("queued_run_target", false);
        ASSERT_TRUE(result.has_value()) << "Non-forced request during recovery action should be queued: "
                                        << result.error().Message();
    }

    TEST_STEP("Release fallback_gated to complete recovery action")
    {
        ASSERT_TRUE(touch_file(gatedReleasePath("fallback_gated")));
    }

    TEST_STEP("Wait for fallback_run_target activation (recovery action completes)")
    {
        WaitForActivation(RunTargetActivationSource::kRecoveryAction, "fallback_run_target");
    }

    TEST_STEP("Wait for queued_run_target activation")
    {
        // After the recovery action completes, the queued request should be processed
        WaitForActivation(RunTargetActivationSource::kStateManagerRequest, "queued_run_target");
    }

    TEST_STEP("Validate active run target")
    {
        const auto result = client->get_active_run_target();
        ASSERT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "queued_run_target");
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
