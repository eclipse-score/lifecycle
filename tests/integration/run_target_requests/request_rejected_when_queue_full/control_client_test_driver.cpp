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
#include <thread>

#include "tests/utils/test_helper/gated_process.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/launch_manager/common/constants.hpp>
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

TEST(RunTargetRequests, RequestRejectedWhenQueueFull)
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

    TEST_STEP("Wait for client connection")
    {
        ASSERT_TRUE(client->wait_for_connection().has_value());
    }

    TEST_STEP("Report running")
    {
        report_running();
    }

    TEST_STEP("Wait for the initial run target to start")
    {
        pop_event([](RunTargetActivationSource source, RunTargetName target) {
            EXPECT_EQ(source, RunTargetActivationSource::kInitialActivation);
            EXPECT_EQ(target, "Startup");
        });
    }

    TEST_STEP("Block run target activation by starting gated process")
    {
        // The gated process will only report running and complete activation
        // after we create a stop file. By blocking activation we can deterministically
        // test the full queue
        const auto result = client->activate_run_target("run_target_gated", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
        ASSERT_TRUE(wait_for_activation_in_progress(*client));
        ASSERT_TRUE(wait_for_file(gatedStartedPath("gated")));
    }

    TEST_STEP("Fill the run target activation request queue")
    {
        for (int i = 0; i < static_cast<int>(score::mw::lifecycle::internal::kPendingActivationQueueCapacity); ++i)
        {
            // Consecutive requests differ so they're not dropped from the queue as duplicates
            const RunTargetName run_target{(i % 2 == 0) ? "run_target_a" : "run_target_b"};
            const auto result = client->activate_run_target(run_target, false);
            ASSERT_TRUE(result.has_value()) << "Request " << i << ": " << result.error().Message();
        }
    }

    TEST_STEP("Repeat the last queued request, it is accepted without using a queue slot")
    {
        const auto result = client->activate_run_target("run_target_b", false);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Request another Run Target while the queue is full")
    {
        const auto result = client->activate_run_target("run_target_a", false);
        ASSERT_FALSE(result.has_value()) << "Request must be rejected while the queue is full";
        EXPECT_EQ(result.error(), ExecErrc::kRequestQueueIsFull);
    }

    TEST_STEP("Forced request for Startup clears the queue")
    {
        const auto result = client->activate_run_target("Startup", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    TEST_STEP("Unblock run target activation")
    {
        ASSERT_TRUE(touch_file(gatedReleasePath("gated")));
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target Startup")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "Startup");
        }
    });

    TEST_STEP("Discarded queue entries are not activated afterwards")
    {
        // The last discarded entry was run_target_b.
        // It would become active if the queue had not been cleared.
        std::this_thread::sleep_for(std::chrono::seconds(1));
        const auto result = client->get_active_run_target();
        ASSERT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "Startup");
    }
}

int main()
{
    TestRunner::Init();
    const auto result = TestRunner::RunTests();
    TestRunner::EndTest();
    TestRunner::WaitForTermination();
    return result;
}
