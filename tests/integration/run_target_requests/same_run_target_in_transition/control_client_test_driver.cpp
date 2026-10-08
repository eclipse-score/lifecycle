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

#include "tests/utils/test_helper/gated_process.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

TEST(RunTargetRequests, SameRunTargetInTransition)
{
    ASSERT_TRUE(check_clean({gatedStartedPath("gated"), gatedReleasePath("gated")}));
    std::unique_ptr<ILmControl> client;

    TEST_STEP("Create client")
    {
        auto client_result = ILmControl::Create("StateManager/LaunchManager/Instance");
        ASSERT_TRUE(client_result.has_value()) << client_result.error().Message();
        client = std::move(client_result).value();
    }

    TEST_STEP("Register callback")
    {
        const auto result = client->register_run_target_activation_callback(push_event);
        ASSERT_TRUE(result.has_value());
    }

    TEST_STEP("Report running")
    {
        report_running();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target Startup")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kInitialActivation);
            EXPECT_EQ(target, "Startup");
        }
    });

    TEST_STEP("Activate run_target_gated")
    {
        const auto result = client->activate_run_target("run_target_gated", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
        ASSERT_TRUE(wait_for_activation_in_progress(*client));
        ASSERT_TRUE(wait_for_file(gatedStartedPath("gated")));
    }

    TEST_STEP("Request run_target_gated again while it is being activated")
    {
        const auto result = client->activate_run_target("run_target_gated", true);
        ASSERT_FALSE(result.has_value()) << "Repeated request during the same activation must be rejected";
        EXPECT_EQ(result.error(), ExecErrc::kInTransitionToSameState);
    }

    TEST_STEP("Release gated")
    {
        ASSERT_TRUE(touch_file(gatedReleasePath("gated")));
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target run_target_gated")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "run_target_gated");
        }
    });

    TEST_STEP("Activate Run Target Off")
    {
        const auto result = client->activate_run_target("Off", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }
}

int main()
{
    return TestRunner(__FILE__, TerminationBehavior::kWait, TerminationNotification::kTestEnd).RunTests();
}
