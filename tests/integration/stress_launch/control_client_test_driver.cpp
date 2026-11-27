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
#include <semaphore.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;
using namespace std::chrono_literals;

namespace
{

/// Name of the named semaphore the stress processes post to.
std::string g_semaphore_name;

/// Number of stress processes, each of them posts once to the semaphore after it was started.
int g_expected_count = 0;

/// Upper bound for the Run Target transitions that start or stop all stress processes.
/// Slightly longer than the transition_timeout_ms in the generated configuration.
constexpr std::chrono::milliseconds kTransitionTimeout{130s};

/// @brief Waits until the named semaphore has been posted exactly `expected` times.
testing::AssertionResult
wait_for_semaphore_count(const std::string& name, const int expected, const std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    sem_t* semaphore = SEM_FAILED;
    int value = 0;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (semaphore == SEM_FAILED)
        {
            // The semaphore is created by the first stress process which posts to it.
            semaphore = ::sem_open(name.c_str(), 0);
        }
        if ((semaphore != SEM_FAILED) && (::sem_getvalue(semaphore, &value) == 0) && (value >= expected))
        {
            break;
        }
        std::this_thread::sleep_for(10ms);
    }
    if (semaphore != SEM_FAILED)
    {
        static_cast<void>(::sem_close(semaphore));
    }

    if (value != expected)
    {
        return testing::AssertionFailure()
               << "Semaphore " << name << " was posted " << value << " times, expected " << expected;
    }
    return testing::AssertionSuccess();
}

std::int64_t elapsed_ms(const std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
}

/// @brief Activates the Run Target `name` and waits until the activation is reported.
/// @return The duration of the transition in milliseconds.
std::int64_t activate_run_target(ILmControl& client, const std::string& name)
{
    const auto start = std::chrono::steady_clock::now();

    const auto result = client.activate_run_target(RunTargetName{name}, true);
    EXPECT_TRUE(result.has_value()) << "Activation of " << name << " failed: " << result.error().Message();

    pop_event(
        [&name](RunTargetActivationSource source, RunTargetName target) {
            EXPECT_EQ(source, RunTargetActivationSource::kStateManagerRequest);
            EXPECT_EQ(target.as_string_view(), name);
        },
        kTransitionTimeout);

    return elapsed_ms(start);
}

}  // namespace

TEST(StressLaunch, ControlClientTestDriver)
{
    ASSERT_FALSE(g_semaphore_name.empty()) << "--semaphore is missing";
    ASSERT_GT(g_expected_count, 0) << "--expected-count is missing";

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

    // Remove a semaphore left over from a previous run, the stress processes are not started yet.
    static_cast<void>(::sem_unlink(g_semaphore_name.c_str()));

    TEST_STEP("Activate Run Target Stress")
    {
        const auto activation_time_ms = activate_run_target(*client, "Stress");
        RecordProperty("activation_time_ms", std::to_string(activation_time_ms));
        std::cout << "[ STRESS   ] Activation of " << g_expected_count + 1 << " processes took " << activation_time_ms
                  << " ms" << std::endl;
    }

    if (HasFailure())
    {
        // The Launch Manager did not start all stress processes.
        return;
    }

    TEST_STEP("Verify that every stress process was started")
    {
        // Each process posts after it was started, which can be slightly later than the Run Target activation.
        EXPECT_TRUE(wait_for_semaphore_count(g_semaphore_name, g_expected_count, 30s));
    }

    TEST_STEP("Activate Run Target Startup to stop all stress processes")
    {
        const auto deactivation_time_ms = activate_run_target(*client, "Startup");
        RecordProperty("deactivation_time_ms", std::to_string(deactivation_time_ms));
        std::cout << "[ STRESS   ] Deactivation of " << g_expected_count + 1 << " processes took "
                  << deactivation_time_ms << " ms" << std::endl;
    }

    static_cast<void>(::sem_unlink(g_semaphore_name.c_str()));
}

int main(int argc, char* argv[])
{
    // Arguments come from the generated configuration: --semaphore <name> --expected-count <count>
    for (int index = 1; (index + 1) < argc; index += 2)
    {
        if (std::strcmp(argv[index], "--semaphore") == 0)
        {
            g_semaphore_name = argv[index + 1];
        }
        else if (std::strcmp(argv[index], "--expected-count") == 0)
        {
            g_expected_count = std::atoi(argv[index + 1]);
        }
    }

    TestRunner::Init();
    const auto result = TestRunner::RunTests();
    TestRunner::EndTest();
    TestRunner::WaitForTermination();
    return result;
}
