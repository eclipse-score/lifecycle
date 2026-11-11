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
#ifndef TESTS_INTEGRATION_ACTIVATION_QUEUE_TEST_FIXTURE_HPP
#define TESTS_INTEGRATION_ACTIVATION_QUEUE_TEST_FIXTURE_HPP

#include <gtest/gtest.h>
#include <unistd.h>

#include <memory>

#include "tests/utils/test_helper/gated_process.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/launch_manager/common/constants.hpp>
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

/// @brief Fixture for tests of a full run target activation request queue.
///
/// SetUp connects a client, blocks run target activation with a gated process
/// and fills the request queue. Each test only has to exercise the full queue.
class ActivationQueueFullTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ASSERT_TRUE(check_clean({gatedStartedPath("gated"), gatedReleasePath("gated")}));
        InitializeClient();

        TEST_STEP("Wait for the initial run target to start")
        {
            WaitForActivation(score::mw::lifecycle::RunTargetActivationSource::kInitialActivation, "Startup");
        }

        TEST_STEP("Block run target activation by starting gated process")
        {
            BlockRunTargetActivation();
        }

        TEST_STEP("Fill the run target activation request queue")
        {
            FillActivationQueue();
        }
    }

    void TearDown() override
    {
        UnblockRunTargetActivation();
    }

    void InitializeClient()
    {
        using namespace score::mw::lifecycle;

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
    }

    void WaitForActivation(
        const score::mw::lifecycle::RunTargetActivationSource& expected_source,
        const score::mw::lifecycle::RunTargetName& expected_run_target) const
    {
        pop_event(
            [&expected_source, &expected_run_target](
                score::mw::lifecycle::RunTargetActivationSource source, score::mw::lifecycle::RunTargetName target) {
                EXPECT_EQ(source, expected_source);
                EXPECT_EQ(target, expected_run_target);
            });
    }

    void FillActivationQueue() const
    {
        for (int i = 0; i < static_cast<int>(score::mw::lifecycle::internal::kPendingActivationQueueCapacity); ++i)
        {
            // Consecutive requests differ so they're not dropped from the queue as duplicates
            const score::mw::lifecycle::RunTargetName run_target{(i % 2 == 0) ? "run_target_a" : "run_target_b"};
            const auto result = client->activate_run_target(run_target, false);
            ASSERT_TRUE(result.has_value()) << "Request " << i << ": " << result.error().Message();
        }
    }

    void BlockRunTargetActivation() const
    {
        // The gated process will only report running and complete activation
        // after we create a release file. By blocking activation we can deterministically
        // test the full queue
        const auto result = client->activate_run_target("run_target_gated", true);
        ASSERT_TRUE(result.has_value()) << result.error().Message();
        ASSERT_TRUE(wait_for_activation_in_progress(*client));
        ASSERT_TRUE(wait_for_file(gatedStartedPath("gated")));
    }

    void UnblockRunTargetActivation() const
    {
        ASSERT_TRUE(touch_file(gatedReleasePath("gated")));
    }

    std::unique_ptr<score::mw::lifecycle::ILmControl> client;
};

#endif  // TESTS_INTEGRATION_ACTIVATION_QUEUE_TEST_FIXTURE_HPP
