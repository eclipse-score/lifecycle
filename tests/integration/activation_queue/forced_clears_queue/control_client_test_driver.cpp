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

#include "tests/integration/activation_queue/full_queue_test_fixture.hpp"

using namespace score::mw::lifecycle;

/// @note See ActivationQueueFullTest::SetUp for test setup.
/// @pre Before running this test, the control client has filled the pending activation queue until full.
/// @pre The last run target to be added to the queue should be "run_target_b",
///   so that we can try duplicating it in this test.
/// @pre The pending activation queue is paused (using a gated process),
///   allowing deterministic behaviour of activate_run_target.
TEST_F(ActivationQueueFullTest, GivenAClientConnection_WhenQueueIsFull_ThenForcedRequestsClearTheQueue)
{
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
