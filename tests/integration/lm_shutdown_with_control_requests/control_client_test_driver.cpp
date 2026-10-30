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
#include <filesystem>

#include "common.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

TEST(LmShutdownControlRequests, ControlClient)
{
    ASSERT_TRUE(check_clean({a_started, a_terminating}));
    std::unique_ptr<ILmControl> client;

    TEST_STEP("Create LmControl client")
    {
        auto client_result = ILmControl::Create("StateManager/LaunchManager/Instance");
        ASSERT_TRUE(client_result.has_value()) << client_result.error().Message();
        client = std::move(client_result).value();
    }

    TEST_STEP("Register run target activation callback")
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

    TEST_STEP("Activate run_target_a")
    {
        const auto result = client->activate_run_target("run_target_a", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Wait for run_target_a activation")
    {
        pop_event([](RunTargetActivationSource source, RunTargetName target) {
            TEST_STEP("Callback for Run Target run_target_a")
            {
                EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
                EXPECT_EQ(target, "run_target_a");
            }
        });
    }

    TEST_STEP("Verify activation of run_target_a")
    {
        EXPECT_TRUE(std::filesystem::exists(a_started)) << "component_a was not started";
    }

    TEST_STEP("Request switch to Off")
    {
        const auto result = client->activate_run_target("Off", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    // Block until the launch manager terminates us as part of its own shutdown
    TestRunner::WaitForTermination();
}

int main()
{
    TestRunner::Init();
    const auto result = TestRunner::RunTests();
    TestRunner::EndTest();
    return result;
}
