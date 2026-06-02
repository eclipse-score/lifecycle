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

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/lifecycle_client.h>

TEST(ComponentD, RunAndVerify)
{
    TEST_STEP("Report running")
    {
        EXPECT_TRUE(touch_file(d_started)) << "failed to deploy file";
        auto result = score::mw::lifecycle::LifecycleClient{}.ReportExecutionState(score::mw::lifecycle::ExecutionState::kRunning);
        EXPECT_TRUE(result.has_value()) << "ReportExecutionState() failed: " << result.error().Message();
    }
    while (!TestRunner::exitRequested)
    {
        pause();
    }
    EXPECT_TRUE(touch_file(d_terminating)) << "Failed to deploy file";
}

int main()
{
    return TestRunner(__FILE__).RunTests();
}
