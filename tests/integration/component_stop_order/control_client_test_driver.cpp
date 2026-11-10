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
#include <string>

#include "tests/utils/test_helper/shm_counter.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

namespace
{
constexpr std::string_view kCounterName = "/component_stop_order_counter";
}  // namespace

TEST(ComponentStopOrder, ControlClientTestDriver)
{
    test::ShmCounter::unlink(kCounterName);
    test::ShmCounter counter{kCounterName};

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

    TEST_STEP("Activate Run Target run_target_b")
    {
        const auto result = client->activate_run_target("run_target_b", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target run_target_b")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "run_target_b");
        }
    });

    TEST_STEP("Wait for components to finish their tests")
    {
        ASSERT_TRUE(wait_for_files(
            {"stop_order_process_a.xml", "stop_order_process_b.xml", "stop_order_process_c.xml"},
            std::chrono::seconds(5)));
    }

    TEST_STEP("Activate Run Target run_target_a")
    {
        const auto result = client->activate_run_target("run_target_a", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target run_target_a")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "run_target_a");
        }
    });

    TEST_STEP("Only c has stopped, and it was the first")
    {
        EXPECT_EQ(counter.get(), 1);
    }

    TEST_STEP("Activate Run Target Startup")
    {
        const auto result = client->activate_run_target("Startup", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target Startup")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "Startup");
        }
    });

    TEST_STEP("b stopped before a: all three stopped in the right order")
    {
        EXPECT_EQ(counter.get(), 3);
    }

    TEST_STEP("Remove the shared counter")
    {
        test::ShmCounter::unlink(kCounterName);
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
