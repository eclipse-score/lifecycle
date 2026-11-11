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

TEST_F(ActivationQueueFullTest, GivenAClientConnection_WhenQueueIsFull_DuplicateUnforcedRequestsAreAccepted)
{
    // Then
    TEST_STEP("Repeat the last queued request, it is accepted without using a queue slot")
    {
        const auto result = client->activate_run_target("run_target_b", false);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
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
