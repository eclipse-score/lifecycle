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
#include <csignal>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include "tests/integration/component_restart/component_restart_files.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/ilm_control.hpp>
#include <score/mw/lifecycle/report_running.h>

using namespace score::mw::lifecycle;

namespace
{
constexpr std::array<std::string_view, 3> kCrashingComponents{{kCrashingA, kCrashingB, kCrashingC}};

/// @return The pid stored by the crashing component `component`, or -1 if it could not be read.
pid_t read_pid(const std::string_view component)
{
    std::ifstream file{pidPath(component)};
    pid_t pid{-1};
    if (!(file >> pid))
    {
        return -1;
    }
    return pid;
}
}  // namespace

TEST(ComponentRestart, ControlClientTestDriver)
{
    const auto restarted_a = restartedPath(kCrashingA);
    const auto restarted_b = restartedPath(kCrashingB);
    const auto restarted_c = restartedPath(kCrashingC);

    ASSERT_TRUE(check_clean({kRecoveryVerifiedFile, restarted_a, restarted_b, restarted_c}));

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

    std::array<pid_t, kCrashingComponents.size()> killed_pids{};
    TEST_STEP("Read the pids of the crashing components")
    {
        for (std::size_t i = 0U; i < kCrashingComponents.size(); ++i)
        {
            killed_pids[i] = read_pid(kCrashingComponents[i]);
            ASSERT_GT(killed_pids[i], 0) << "Could not read pid of " << kCrashingComponents[i];
        }
    }

    TEST_STEP("Kill all crashing components at the same time")
    {
        for (const auto pid : killed_pids)
        {
            EXPECT_EQ(kill(pid, SIGKILL), 0) << "Failed to kill pid " << pid;
        }
    }

    TEST_STEP("Verify the Run Target Startup stays active until all crashing components have been restarted")
    {
        constexpr auto kTimeout = std::chrono::seconds(10);
        constexpr auto kPollInterval = std::chrono::milliseconds(10);
        const auto deadline = std::chrono::steady_clock::now() + kTimeout;
        const auto all_restarted = [&]() {
            return std::filesystem::exists(restarted_a) && std::filesystem::exists(restarted_b) &&
                   std::filesystem::exists(restarted_c);
        };

        do
        {
            const auto result = client->get_active_run_target();
            ASSERT_TRUE(result.has_value()) << result.error().Message();
            ASSERT_EQ(result.value(), "Startup");
            std::this_thread::sleep_for(kPollInterval);
        } while (!all_restarted() && (std::chrono::steady_clock::now() < deadline));

        ASSERT_TRUE(all_restarted()) << "Not all crashing components were restarted within the timeout";
    }

    TEST_STEP("Verify the crashing components run in new processes")
    {
        for (std::size_t i = 0U; i < kCrashingComponents.size(); ++i)
        {
            const auto pid = read_pid(kCrashingComponents[i]);
            ASSERT_GT(pid, 0) << "Could not read pid of " << kCrashingComponents[i];
            EXPECT_NE(pid, killed_pids[i]) << kCrashingComponents[i] << " was not restarted";
        }
    }

    // A switch to the fallback Run Target starts fail_test, which fails the test.
    TEST_STEP("Signal that the recovery has been verified")
    {
        ASSERT_TRUE(touch_file(kRecoveryVerifiedFile));
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
