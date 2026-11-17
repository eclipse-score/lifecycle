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
#include <csignal>
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
constexpr std::string_view kHanging = "crashing_hanging";
constexpr std::string_view kExhausted = "crashing_exhausted";

/// @brief Must match activation_retry_attempts of crashing_exhausted in the configuration.
constexpr int kExhaustedRetryAttempts = 1;

/// @brief Covers the ready_timeout_ms of crashing_hanging (5 s) with margin. The in-progress restart of
/// crashing_hanging is not interrupted by the stop request, because waiting for a Reporting process to report
/// running ignores the stop token. The graph therefore stays in kAborting until that restart times out.
constexpr auto kFallbackTimeout = std::chrono::seconds(15);

/// @return The number stored in `file_path`, or -1 if it could not be read.
long read_value(const std::string& file_path)
{
    std::ifstream file{file_path};
    long value{-1};
    if (!(file >> value))
    {
        return -1;
    }
    return value;
}

/// @return True if the process `pid` has terminated within `timeout`.
bool wait_for_process_exit(const pid_t pid, const std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (kill(pid, 0) == 0)
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}
}  // namespace

TEST(ComponentRestartRecoveryExhausted, ControlClientTestDriver)
{
    ASSERT_TRUE(check_clean({hangingPath(kHanging)}));

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

    TEST_STEP("Kill crashing_hanging and wait until its restart is in progress")
    {
        const auto pid = static_cast<pid_t>(read_value(pidPath(kHanging)));
        ASSERT_GT(pid, 0) << "Could not read pid of " << kHanging;
        ASSERT_EQ(kill(pid, SIGKILL), 0);
        ASSERT_TRUE(wait_for_file(hangingPath(kHanging), std::chrono::seconds(5)));
    }

    TEST_STEP("Verify the Run Target Startup is still active during the recovery")
    {
        const auto result = client->get_active_run_target();
        ASSERT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "Startup");
    }

    const auto hanging_restart_pid = static_cast<pid_t>(read_value(pidPath(kHanging)));

    TEST_STEP("Kill crashing_exhausted while the restart of crashing_hanging is in progress")
    {
        const auto pid = static_cast<pid_t>(read_value(pidPath(kExhausted)));
        ASSERT_GT(pid, 0) << "Could not read pid of " << kExhausted;
        ASSERT_EQ(kill(pid, SIGKILL), 0);
    }

    pop_event(
        [](RunTargetActivationSource source, RunTargetName target) {
            TEST_STEP("Callback for the fallback Run Target")
            {
                EXPECT_EQ(source, RunTargetActivationSource::kRecoveryAction);
                EXPECT_EQ(target, "fallback_run_target");
            }
        },
        kFallbackTimeout);

    TEST_STEP("Verify the fallback Run Target is active")
    {
        const auto result = client->get_active_run_target();
        ASSERT_TRUE(result.has_value()) << result.error().Message();
        EXPECT_EQ(result.value(), "fallback_run_target");
    }

    TEST_STEP("Verify crashing_exhausted used exactly its retry budget")
    {
        // One initial start plus activation_retry_attempts + 1 restart attempts.
        EXPECT_EQ(read_value(startCountPath(kExhausted)), 1 + kExhaustedRetryAttempts + 1);
    }

    TEST_STEP("Verify the in-progress restart of crashing_hanging was stopped and not retried")
    {
        EXPECT_EQ(read_value(startCountPath(kHanging)), 2);
        ASSERT_GT(hanging_restart_pid, 0);
        EXPECT_TRUE(wait_for_process_exit(hanging_restart_pid, std::chrono::seconds(5)))
            << "Restarted crashing_hanging process is still running";
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
