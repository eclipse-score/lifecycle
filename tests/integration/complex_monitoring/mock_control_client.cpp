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
#include <unistd.h>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

TEST(ComplexMonitoring, ControlClientMock)
{
    const auto client = score::mw::lifecycle::ILmControl::Create("StateManager/LaunchManager/Instance");
    ASSERT_TRUE(client.has_value());

    ASSERT_TRUE(check_clean({test_end_location, fallback_file}));

    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }

    TEST_STEP("Launch monitored process")
    {
        auto result = client->get()->activate_run_target("run_target_complex_monitoring");
        EXPECT_TRUE(result.has_value()) << "Activating target run_target_complex_monitoring failed: "
                                        << result.error().Message();
    }
    // Wait for health monitoring to fail and recovery to trigger
    sleep(2);
    TEST_STEP("Verify state changed to fallback run target")
    {
        // workaround to detect we're in fallback
        EXPECT_TRUE(std::filesystem::exists(fallback_file)) << "Fallback run target was not activated";
    }
    TEST_STEP("Activate Off run target")
    {
        const auto result = client->get()->activate_run_target("Off");
        EXPECT_TRUE(result.has_value()) << "Expected Off activation to succeed";
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
