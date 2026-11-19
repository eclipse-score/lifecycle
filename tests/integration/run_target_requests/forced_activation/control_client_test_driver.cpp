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
#include <chrono>

#include "tests/utils/fixtures/activation.hpp"
#include "tests/utils/test_helper/gated_process.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

TEST_F(
    ActivationFixture,
    GivenAClientConnection_WhenForcedActivationIsRequested_ThenOngoingTransitionAbortedAndQueueIsCleared)
{
    ASSERT_TRUE(check_clean({gatedStartedPath("gated"), gatedReleasePath("gated")}));

    InitializeClient();

    TEST_STEP("Wait for the initial run target to start")
    {
        WaitForActivation(RunTargetActivationSource::kInitialActivation, "Startup");
    }

    TEST_STEP("Block run target activation by starting gated process")
    {
        BlockRunTargetActivation();
    }

    TEST_STEP("Fill the run target activation request queue")
    {
        FillActivationQueue();
    }

    TEST_STEP("Forced request for Startup clears the queue")
    {
        const auto result = client->activate_run_target("Startup", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Unblock run target activation")
    {
        UnblockRunTargetActivation();
    }

    TEST_STEP("Callback for Run Target Startup")
    {
        WaitForActivation(RunTargetActivationSource::kStateManagerRequest, "Startup");
    }

    TEST_STEP("Check that discarded queue entries are not activated afterwards")
    {
        expect_no_events(
            [](RunTargetActivationSource, RunTargetName target) {
                ADD_FAILURE() << "Received activation request " << target;
            },
            std::chrono::milliseconds(1000));
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
