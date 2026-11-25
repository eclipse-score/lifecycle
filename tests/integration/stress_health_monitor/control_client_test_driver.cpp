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
#include <memory>

#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;
using namespace std::chrono_literals;

namespace
{
/// The monitored process sends heartbeats for 10 s after it reported running. No recovery action is expected
/// during this time, with a margin for the start of the monitored process.
constexpr std::chrono::milliseconds kNoRecoveryDuration{8s};

/// Time for the health monitor and the Launch Manager to detect the missing heartbeats after they stopped.
constexpr std::chrono::milliseconds kRecoveryTimeout{10s};
}  // namespace

TEST(StressHealthMonitor, ControlClientTestDriver)
{
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

    TEST_STEP("Activate Run Target Stress")
    {
        const auto result = client->activate_run_target("Stress", true);
        ASSERT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target Stress")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "Stress");
        }
    });

    // A missed heartbeat makes the health monitor withhold its alive notifications. The Launch Manager then
    // executes the recovery action, which activates the fallback Run Target and reports it here.
    TEST_STEP("Expect no recovery action while the heartbeats are sent")
    {
        expect_no_events(kNoRecoveryDuration);
    }

    // When the heartbeats stop, the missing heartbeats have to be detected with 1024 monitors as well.
    pop_event(
        [](RunTargetActivationSource source, RunTargetName target) {
            TEST_STEP("Callback for Run Target fallback")
            {
                EXPECT_EQ(source, RunTargetActivationSource::kRecoveryAction);
                EXPECT_EQ(target, "fallback_run_target");
            }
        },
        kRecoveryTimeout);
}

int main()
{
    TestRunner::Init();
    const auto result = TestRunner::RunTests();
    TestRunner::EndTest();
    TestRunner::WaitForTermination();
    return result;
}
