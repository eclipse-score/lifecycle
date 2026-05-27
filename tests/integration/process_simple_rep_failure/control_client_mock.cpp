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
#include <score/lcm/control_client.h>
#include <score/lcm/lifecycle_client.h>

// Given a correct configuration with:
//   - An initial Run Target named "Startup" containing 2 components named "component_initial" and "control_client_mock"
//   - A Run Target named "run_target_app_does_report_krunning_in_time" containing "control_client_mock" and "component_does_report_krunning_in_time"
//   - A Run Target named "run_target_app_does_not_report_krunning_in_time" containing "control_client_mock" and "component_does_not_report_krunning_in_time"

TEST(RecoveryActionSimpleRepFailure, ControlClientMock)
{
    score::lcm::ControlClient client;

    ASSERT_TRUE(check_clean({test_end_location, fallback_file}));

    // Establish communication with launch manager
    TEST_STEP("Report kRunning from ControlClientMock")
    {
        auto result = score::lcm::LifecycleClient{}.ReportExecutionState(score::lcm::ExecutionState::kRunning);
        ASSERT_TRUE(result.has_value()) << "ReportExecutionState() failed: " << result.error().Message();
    }
    // Start the run target run_target_app_does_report_krunning_in_time
    TEST_STEP("Activate RunTarget run_target_app_does_report_krunning_in_time")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("run_target_app_does_report_krunning_in_time").Get(stop_token);
        EXPECT_TRUE(result.has_value()) << "Activating target run_target_app_does_report_krunning_in_time failed: " << result.error().Message();
    }
    sleep(2);
    // Start the run target run_target_app_does_report_krunning_in_time
    TEST_STEP("Activate RunTarget run_target_app_does_not_report_krunning_in_time")
    {
        score::cpp::stop_token stop_token;
        auto result = client.ActivateRunTarget("run_target_app_does_not_report_krunning_in_time").Get(stop_token);
        EXPECT_FALSE(result.has_value()) << "Activating target run_target_app_does_not_report_krunning_in_time failed: " << result.error().Message();
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
