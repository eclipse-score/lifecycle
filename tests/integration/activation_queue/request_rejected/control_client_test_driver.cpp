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

TEST_F(ActivationQueueFullTest, GivenAClientConnection_WhenQueueIsFull_NewUnforcedRequestsAreRejected)
{
    // NOTE: See ActivationQueueFullTest::SetUp for Given and Then stages
    TEST_STEP("Request another Run Target while the queue is full")
    {
        const auto result = client->activate_run_target("run_target_a", false);
        ASSERT_FALSE(result.has_value()) << "Request must be rejected while the queue is full";
        EXPECT_EQ(result.error(), ExecErrc::kRequestQueueIsFull);
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
