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
#include <score/mw/launch_manager/common/constants.hpp>

using namespace score::mw::lifecycle;

TEST_F(ActivationFixture, GivenAClientConnection_WhenActivationQueueIsFull_ThenRequestsAreRejected)
{
    ASSERT_TRUE(check_clean({gatedStartedPath("gated"), gatedReleasePath("gated")}));

    InitializeClient();

    TEST_STEP("Wait for Startup activation")
    {
        WaitForActivation(RunTargetActivationSource::kInitialActivation, "Startup");
    }

    TEST_STEP("Block the activation queue with gated run target")
    {
        BlockRunTargetActivation();
    }

    TEST_STEP("Fill the queue with max requests")
    {
        // Queue capacity is 10, so we should be able to queue exactly 10 items
        for (int i = 0; i < score::mw::lifecycle::internal::kPendingActivationQueueCapacity; ++i)
        {
            const std::string_view run_target_view = "run_target_" + std::to_string(i);
            const RunTargetName run_target_name{run_target_view};
            const auto result = client->activate_run_target(run_target_name, false);
            ASSERT_TRUE(result.has_value())
                << "Failed to queue " << run_target_name << ": " << result.error().Message();
        }
    }

    TEST_STEP("Attempt to queue another activation")
    {
        const auto result = client->activate_run_target("run_target_overflow", false);
        ASSERT_FALSE(result.has_value()) << "11th activation should be rejected when queue is full";
        EXPECT_EQ(result.error(), ExecErrc::kRequestQueueIsFull) << "Expected kQueueFull error";
    }

    TEST_STEP("Release gated run target to process queue")
    {
        UnblockRunTargetActivation();
    }

    TEST_STEP("Wait for gated run_target_gated activation")
    {
        WaitForActivation(RunTargetActivationSource::kStateManagerRequest, "run_target_gated");
    }

    TEST_STEP("Verify all queued activations occur in FIFO order")
    {
        // The queue should process in the order they were added: run_target_0 through run_target_9
        for (int i = 0; i < score::mw::lifecycle::internal::kPendingActivationQueueCapacity; ++i)
        {
            const std::string_view expected_run_target_view = "run_target_" + std::to_string(i);
            const RunTargetName expected_run_target{expected_run_target_view};
            WaitForActivation(RunTargetActivationSource::kStateManagerRequest, expected_run_target);
        }
    }

    TEST_STEP("Validate final active run target")
    {
        const auto result = client->get_active_run_target();
        ASSERT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "run_target_9");
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
