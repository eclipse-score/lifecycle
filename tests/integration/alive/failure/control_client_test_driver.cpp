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

// Given a correct configuration with:
//   - An initial Run Target "Startup" containing "control_client_test_driver"
//   - Run Targets "cpp_failure" and "rust_failure", which contain a C++ / Rust component that stops reporting alive
//     after a few notifications
//   - A Run Target "success", which contains a C++ and a Rust component that keep reporting alive
//   - A fallback Run Target that touches the fallback file

using namespace score::mw::lifecycle;
using namespace std::chrono_literals;

namespace
{
/// Time during which a healthy alive supervision must not trigger the fallback Run Target (many evaluation cycles).
constexpr auto kNoFallbackObservationTime = 1500ms;
}  // namespace

TEST(AliveFailure, ControlClientTestDriver)
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

    // Activates a Run Target containing a component that stops reporting alive and verifies the fallback
    // Run Target is activated as a consequence.
    const auto expect_fallback = [&client](std::string_view run_target_name) {
        const RunTargetName run_target{run_target_name};
        const std::string name{run_target_name};

        TEST_STEP("Activate Run Target " + name)
        {
            const auto result = client->activate_run_target(run_target, true);
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
    };

    expect_fallback("cpp_failure");
    expect_fallback("rust_failure");

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
