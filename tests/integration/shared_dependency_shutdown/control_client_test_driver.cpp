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

#include "tests/integration/shared_dependency_shutdown/common.hpp"
#include "tests/utils/test_helper/shm_counter.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

TEST(SharedDependencyShutdown, ControlClientTestDriver)
{
    // clean the counters
    test::ShmCounter::unlink(kStartCounterName);
    test::ShmCounter::unlink(kStopCounterName);
    test::ShmCounter::unlink(kViolationCounterName);
    test::ShmCounter stop_counter{kStopCounterName};
    test::ShmCounter violation_counter{kViolationCounterName};

    std::unique_ptr<ILmControl> client;

    TEST_STEP("Create client")
    {
        auto client_result = ILmControl::Create("StateManager/LaunchManager/Instance");
        ASSERT_TRUE(client_result.has_value()) << client_result.error();
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

    TEST_STEP("Launch Run Target run_target_diamond")
    {
        const auto result = client->activate_run_target("run_target_diamond", true);
        EXPECT_TRUE(result.has_value()) << result.error();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target run_target_diamond")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "run_target_diamond");
        }
    });

    TEST_STEP("Wait for components to finish their tests")
    {
        ASSERT_TRUE(wait_for_files(
            {"component_shared_process.xml", "dependent_process_fast.xml", "dependent_process_slow.xml"},
            std::chrono::seconds(5)));
    }

    TEST_STEP("Every component took exactly one start ticket")
    {
        test::ShmCounter start_counter{kStartCounterName};
        EXPECT_EQ(start_counter.get(), kComponentCount);
    }

    TEST_STEP("Nothing has stopped yet")
    {
        EXPECT_EQ(stop_counter.get(), 0);
    }

    // component_fast and component_slow both depend on component_shared and are stopped in
    // parallel. component_fast exits quickly, component_slow keeps running for a while after
    // SIGTERM. component_shared must not be asked to stop until both of them are gone.
    TEST_STEP("Activate Startup Run Target, which stops both dependents and then the shared component")
    {
        const auto result = client->activate_run_target("Startup", true);
        EXPECT_TRUE(result.has_value()) << result.error();
    }

    pop_event([](RunTargetActivationSource source, RunTargetName target) {
        TEST_STEP("Callback for Run Target Startup")
        {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target, "Startup");
        }
    });

    TEST_STEP("component_shared was not asked to stop before all of its dependents had exited")
    {
        EXPECT_EQ(violation_counter.get(), 0)
            << "component_shared received SIGTERM while fewer than its " << kSharedDependentCount
            << " dependents had exited: a dependent that had only begun its graceful shutdown was "
               "already treated as stopped";
    }

    TEST_STEP("All three components stopped")
    {
        EXPECT_EQ(stop_counter.get(), kComponentCount);
    }

    TEST_STEP("Remove the shared counters")
    {
        test::ShmCounter::unlink(kStartCounterName);
        test::ShmCounter::unlink(kStopCounterName);
        test::ShmCounter::unlink(kViolationCounterName);
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
