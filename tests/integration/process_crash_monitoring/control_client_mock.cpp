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
#include <fcntl.h>
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/lifecycle_client.h>


// Given a correct configuration with:
//   - An initial Run Target named "Startup" containing "control_client_mock"
//   - A Run Target named "run_target_crashing_app_on_runtime" containing "control_client_mock" and
//     "component_crashing_on_runtime"

TEST(ProcessCrashMonitoring, ControlClientMock)
{
    score::mw::lifecycle::ControlClient client;
    
    ASSERT_TRUE(check_clean({test_end_location, fallback_file}));
    // Establish communication with launch manager
    TEST_STEP("Report kRunning")
    {
        auto result = score::mw::lifecycle::LifecycleClient{}.ReportExecutionState(score::mw::lifecycle::ExecutionState::kRunning);
        ASSERT_TRUE(result.has_value()) << "ReportExecutionState() failed: " << result.error().Message();
    }

    // We have to wait for the initial state transition to fully complete, otherwise unexpected failures can occur
    // Tracked in https://github.com/eclipse-score/lifecycle/issues/198
    sleep(1);
    
    TEST_STEP("Start crashing process")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("run_target_crashing_app_on_runtime").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating target run_target_crashing_app_on_runtime failed: "
                                        << result.error().Message();
    }
    // When the process crashes
    sleep(2);
    // Then
    TEST_STEP("Verify state changed to fallback run target")
    {
        // workaround to detect we're in fallback
        EXPECT_TRUE(std::filesystem::exists(fallback_file)) << "Fallback run target was not activated";
    }
    TEST_STEP("Activate RunTarget Off")
    {
        client.ActivateRunTarget("Off");
    }
}

int main(int argc, char** argv)
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
