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

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <thread>

#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

/// @file
/// @warning this control client requires the following RTs:
///   - An initial Run Target "Startup" containing "control_client_test_driver"
///   - Run Target "too_few", which contains a component that stops reporting alive after a few notifications
///   - Run Target "too_many", which contains a component that reports alive more often than allowed
///   - Run Target "success", which contains a component that reports alive within the allowed range
///   - A fallback Run Target that touches the fallback file

using namespace score::mw::lifecycle;
using namespace std::chrono_literals;

namespace
{
/// @brief Time to wait untill we're confident it's working.
constexpr auto kNoFallbackObservationTime = 1500ms;

/// @brief Activates a Run Target that shall error, and verifies that fallback
///        is reached.
/// @param client  The client.
/// @param run_target_name  The run target to switch to.
void ExpectFallback(ILmControl& client, std::string_view run_target_name)
{
    const RunTargetName run_target{run_target_name};
    const std::string name{run_target_name};

    TEST_STEP("Activate Run Target " + name)
    {
        const auto result = client.activate_run_target(run_target, true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([run_target, name](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target " + name)
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, run_target);
        }
    });

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for fallback Run Target")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kRecoveryAction);
            EXPECT_EQ(target, "fallback_run_target");
        }
    });

    TEST_STEP("Verify fallback Run Target was activated")
    {
        EXPECT_TRUE(wait_for_file(fallback_file)) << "Fallback Run Target should have been activated";
        std::filesystem::remove(fallback_file);
    }
}
}  // namespace

TEST(AliveSupervision, ControlClientTestDriver)
{
    ASSERT_TRUE(check_clean({fallback_file}));

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

    TEST_STEP("Wait for client connection")
    {
        ASSERT_TRUE(client->wait_for_connection().has_value());
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

    ExpectFallback(*client, "too_few");
    ExpectFallback(*client, "too_many");

    TEST_STEP("Activate Run Target success")
    {
        const auto result = client->activate_run_target("success", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target success")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "success");
        }
    });

    TEST_STEP("Verify fallback Run Target is not activated while alive is reported")
    {
        std::this_thread::sleep_for(kNoFallbackObservationTime);
        EXPECT_FALSE(std::filesystem::exists(fallback_file)) << "Fallback Run Target should not have been activated";
    }
}

int main()
{
    TestRunner::Init();
    const auto result = TestRunner::RunTests();
    TestRunner::EndTest();
    return result;
}
