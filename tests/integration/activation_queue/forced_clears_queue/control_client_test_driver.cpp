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

#include "tests/integration/activation_queue/test_fixture.hpp"

using namespace score::mw::lifecycle;

TEST_F(ActivationQueueFullTest, GivenAClientConnection_WhenQueueIsFull_ForcedRequestsClearTheQueue)
{
    // Then
    TEST_STEP("Forced request for Startup clears the queue")
    {
        const auto result = client->activate_run_target("Startup", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Unblock run target activation")
    {
        ASSERT_TRUE(touch_file(gatedReleasePath("gated")));
    }

    TEST_STEP("Callback for Run Target Startup")
    {
        WaitForActivation(RunTargetActivationSource::kStateManagerRequest, "Startup");
    }

    TEST_STEP("Discarded queue entries are not activated afterwards")
    {
        // We do not expect to see any more events,
        // as the forced activation to startup should clear the queue
        ExpectNoEvents(
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
