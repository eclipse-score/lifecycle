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
#include <array>
#include <chrono>
#include <filesystem>

#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

namespace
{
constexpr std::array<std::string_view, 3> kComponentIds{{"a", "b", "c"}};
}  // namespace

using namespace score::mw::lifecycle;

TEST(ParallelLaunch, ControlClientTestDriver)
{
    for (const auto id : kComponentIds)
    {
        ASSERT_TRUE(check_clean({"start_" + std::string{id}, "running_" + std::string{id}}, false));
    }

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

    TEST_STEP("Launch parallel Run Target")
    {
        const auto result = client->activate_run_target("run_target_parallel_launch", true);
        EXPECT_TRUE(result.has_value()) << result.error().Message();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target run_target_parallel_launch")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "run_target_parallel_launch");
        }
    });

    // The Run Target is active as soon as every component has reported running, so a component can still be in the
    // middle of its test. Terminating it then can exceed the shutdown timeout and get it killed before it has written
    // its result. Wait until each component has finished its test (it writes its xml last) before terminating them.
    TEST_STEP("Wait for components to finish their tests")
    {
        ASSERT_TRUE(wait_for_files(
            {"start_a",
             "start_b",
             "start_c",
             "running_a",
             "running_b",
             "running_c",
             "component_parallel_launch_a.xml",
             "component_parallel_launch_b.xml",
             "component_parallel_launch_c.xml"},
            std::chrono::seconds(5)));
    }

    // Activate Run Target Startup again, to be sure that the termination of all components has been finished and the
    // files and its timestamps can be evaluated in the next test step.
    TEST_STEP("Activate Startup Run Target again")
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

    TEST_STEP("Verify all components started before any reported running")
    {
        std::filesystem::file_time_type max_start = std::filesystem::file_time_type::min();
        std::filesystem::file_time_type min_running = std::filesystem::file_time_type::max();
        for (const auto id : kComponentIds)
        {
            const std::filesystem::path start_file{"start_" + std::string{id}};
            const std::filesystem::path running_file{"running_" + std::string{id}};
            max_start = std::max(max_start, std::filesystem::last_write_time(start_file));
            min_running = std::min(min_running, std::filesystem::last_write_time(running_file));
        }
        // Parallel launch: the last component to start did so before the first
        // one finished sleeping and reported running. Sequential launch would
        // violate this because a later component starts only after an earlier
        // one is already running.
        EXPECT_LT(max_start, min_running) << "Components were not launched in parallel";
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
