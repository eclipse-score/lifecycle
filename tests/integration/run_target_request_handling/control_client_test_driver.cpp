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

#include <chrono>
#include <filesystem>
#include <thread>

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

namespace
{
/// Waits until an accepted request has actually started its transition.
testing::AssertionResult wait_for_activation_in_progress(ILmControl& client)
{
    constexpr auto kPollInterval = std::chrono::milliseconds(10);
    constexpr int kMaxPolls = 300;

    for (int poll = 0; poll < kMaxPolls; ++poll)
    {
        const auto result = client.get_active_run_target();
        if (!result.has_value() && (result.error() == ExecErrc::kActivationInProgress))
        {
            return testing::AssertionSuccess();
        }
        std::this_thread::sleep_for(kPollInterval);
    }
    return testing::AssertionFailure() << "Activation did not reach the in-progress state";
}

/// Waits until `file` exists.
testing::AssertionResult wait_for_file(const std::string& file)
{
    constexpr auto kPollInterval = std::chrono::milliseconds(10);
    constexpr int kMaxPolls = 300;

    for (int poll = 0; poll < kMaxPolls; ++poll)
    {
        if (std::filesystem::exists(file))
        {
            return testing::AssertionSuccess();
        }
        std::this_thread::sleep_for(kPollInterval);
    }
    return testing::AssertionFailure() << file << " was not created";
}
}  // namespace

TEST(RunTargetRequestHandling, ControlClient)
{
    ASSERT_TRUE(check_clean(
        {started_file(gated_a),
         release_file(gated_a),
         running_file(gated_a),
         started_file(gated_b),
         release_file(gated_b),
         running_file(gated_b)}));
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

    TEST_STEP("Request a Run Target that does not exist")
    {
        const auto result = client->activate_run_target("run_target_does_not_exist", true);
        ASSERT_FALSE(result.has_value()) << "Request for an unknown Run Target must be rejected";
        EXPECT_EQ(result.error(), ExecErrc::kRunTargetDoesntExist);
    }

    TEST_STEP("Activate run_target_a")
    {
        const auto result = client->activate_run_target("run_target_a", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
        ASSERT_TRUE(wait_for_activation_in_progress(*client));
    }

    TEST_STEP("Request run_target_a again while it is being activated")
    {
        const auto result = client->activate_run_target("run_target_a", true);
        ASSERT_FALSE(result.has_value()) << "Repeated request during the same activation must be rejected";
        EXPECT_EQ(result.error(), ExecErrc::kInTransitionToSameState);
    }

    TEST_STEP("Release gated_a")
    {
        ASSERT_TRUE(touch_file(release_file(gated_a)));
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target run_target_a")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "run_target_a");
        }
    });

    TEST_STEP("Request run_target_a again while it is active")
    {
        const auto result = client->activate_run_target("run_target_a", true);
        ASSERT_FALSE(result.has_value()) << "Request for the already active Run Target must be rejected";
        EXPECT_EQ(result.error(), ExecErrc::kAlreadyInState);
    }

    TEST_STEP("Activate run_target_b")
    {
        const auto result = client->activate_run_target("run_target_b", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
        ASSERT_TRUE(wait_for_activation_in_progress(*client));
        // gated_a is deactivated first; wait until gated_b is launched.
        ASSERT_TRUE(wait_for_file(started_file(gated_b)));
    }

    TEST_STEP("Request Startup while run_target_b is being activated")
    {
        const auto result = client->activate_run_target("Startup", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Release gated_b")
    {
        // Lets the replaced activation settle.
        ASSERT_TRUE(touch_file(release_file(gated_b)));
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target Startup, not for the replaced run_target_b")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "Startup");
        }
    });

    TEST_STEP("Validate active Run Target")
    {
        const auto result = client->get_active_run_target();
        ASSERT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "Startup");
    }

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
