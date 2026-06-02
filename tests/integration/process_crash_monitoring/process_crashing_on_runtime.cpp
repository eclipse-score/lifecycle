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

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/lifecycle_client.h>

TEST(ProcessCrashMonitoring, CrashingProcess)
{
    TEST_STEP("Report kRunning")
    {
        auto result = score::mw::lifecycle::LifecycleClient{}.ReportExecutionState(score::mw::lifecycle::ExecutionState::kRunning);
        ASSERT_TRUE(result.has_value()) << "ReportExecutionState() failed: " << result.error().Message();
    }
}

int main()
{
    TestRunner(__FILE__, TerminationBehavior::kContinue).RunTests();
    // Plenty of time to output the XML file and for LM to complete run target activation
    sleep(1);
    std::cout << "Process crashing..." << std::endl;
    std::abort();
}
