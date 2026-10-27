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
#ifndef TESTS_UTILS_TEST_RUNNER_HPP
#define TESTS_UTILS_TEST_RUNNER_HPP

#include <gtest/gtest.h>
#include <unistd.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

/// @return File path to an xml adjacent to the input file path
inline std::string xmlPath(const std::string_view file)
{
    return std::filesystem::path{file}.filename().stem().string() + ".xml";
}

#define TEST_STEP(message)                                                                 \
    for (bool once = (std::cout << "[ STEP     ] " << (message) << std::endl, true); once; \
         (std::cout << "[ END STEP ] " << (message) << std::endl), once = false)

/// @brief Helper to setup, run, and clean up GTEST tests. All members are static; call Init() first in main().
class TestRunner
{
    static_assert(std::atomic<bool>::is_always_lock_free, "g_exit_requested is written from a signal handler");

    static void signalHandler([[maybe_unused]] int signal)
    {
        g_exit_requested = true;
    }

    static void installHandler(const int signal_number)
    {
        [[maybe_unused]] const auto previous = std::signal(signal_number, signalHandler);
        assert(previous != SIG_ERR);
    }

  public:
    TestRunner() = delete;

    /// @brief True if the test process has received SIGINT or SIGTERM
    inline static std::atomic<bool> g_exit_requested = false;

    /// @brief Installs signal handlers and initializes GTEST.
    /// @param[in] test_path location to write the GTEST xml file (__FILE__)
    static void Init(const char* test_path = __builtin_FILE())
    {
        installHandler(SIGINT);
        installHandler(SIGTERM);

        ::testing::GTEST_FLAG(output) = "xml:" + xmlPath(test_path);
        ::testing::GTEST_FLAG(brief) = true;
        ::testing::InitGoogleTest();
    }

    /// @brief Run all tests.
    static int RunTests()
    {
        return RUN_ALL_TESTS();
    }

    /// @brief Signals LM that the test has completed.
    static void EndTest()
    {
        [[maybe_unused]] const int result = kill(getppid(), SIGTERM);
        assert(result == 0);
    }

    /// @brief Blocks until SIGINT or SIGTERM is received.
    /// @details Using polling instead of atomics to reduce test flakiness.
    static void WaitForTermination()
    {
        while (!g_exit_requested)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};

#endif  // TESTS_UTILS_TEST_RUNNER_HPP
