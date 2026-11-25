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

TEST_F(ActivationFixture, GivenAClientConnection_WhenRequestReceivedDuringRecovery_ThenForcedRequestCancelsRecovery)
{
    ASSERT_TRUE(check_clean(
        {gatedStartedPath("failing_component"),
         gatedReleasePath("failing_component"),
         gatedStartedPath("fallback_gated"),
         gatedReleasePath("fallback_gated")}));

    std::unique_ptr<ILmControl> client;

    TEST_STEP("Initialize client")
    {
        auto maybe_client = InitializeClient();
        ASSERT_TRUE(maybe_client.has_value());
        client = std::move(maybe_client).value();
    }

    TEST_STEP("Report running")
    {
        report_running();
    }

    TEST_STEP("Wait for Startup activation")
    {
        WaitForActivation(RunTargetActivationSource::kInitialActivation, "Startup");
    }

    TEST_STEP("Force activate run_target_with_failing_component and wait for the file signalling recovery action start")
    {
        BlockRunTargetActivation(*client, "run_target_with_failing_component", "fallback_gated");
    }

    TEST_STEP("Send forced request for forced_run_target during recovery action")
    {
        // This forced request should cancel the ongoing recovery action
        const auto result = client->activate_run_target("forced_run_target", true);
        ASSERT_TRUE(result.has_value()) << "Forced request during recovery action should be accepted: "
                                        << result.error().Message();
    }

    TEST_STEP("Wait for forced_run_target activation (recovery cancelled)")
    {
        WaitForActivation(RunTargetActivationSource::kStateManagerRequest, "forced_run_target");
    }

    TEST_STEP("Validate active run target")
    {
        const auto result = client->get_active_run_target();
        ASSERT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "forced_run_target");
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
