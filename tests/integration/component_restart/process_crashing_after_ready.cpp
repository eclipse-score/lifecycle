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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include "tests/integration/component_restart/component_restart_files.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{
std::string g_identifier;
int g_failed_restart_attempts = 0;
bool g_hang = false;
int g_start_count = 0;

int read_start_count(const std::string& file_path)
{
    std::ifstream file{file_path};
    int count = 0;
    if (file >> count)
    {
        return count;
    }
    // file does not exist yet, first startup
    return 0;
}

void write_to_file(const std::string& file_path, const long value)
{
    std::ofstream file{file_path};
    file << value;
}
}  // namespace

TEST(ComponentRestart, ProcessCrashingAfterReady)
{
    TEST_STEP("Verify the number of start attempts")
    {
        // One initial start, the failed restart attempts, and this successful restart.
        EXPECT_EQ(g_start_count, 2 + g_failed_restart_attempts);
    }
}

/// Usage: process_crashing_after_ready <identifier> [failed_restart_attempts] [hang]
///
/// Every start stores its pid in <identifier>.pid, so the control client can kill it.
/// 1st start:  reports running and stays running until the control client kills it.
/// Next `failed_restart_attempts` starts: terminate before reporting running.
/// Next start: reports running and stays running until SIGTERM. Only this start writes the
///             report <identifier>.xml.
///             With "hang", it instead creates <identifier>_hanging and never reports running, so the
///             restart stays in progress until the Launch Manager stops it. No report is written.
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <identifier> [failed_restart_attempts] [hang]" << std::endl;
        return EXIT_FAILURE;
    }
    g_identifier = argv[1];
    g_failed_restart_attempts = (argc > 2) ? std::atoi(argv[2]) : 0;
    g_hang = (argc > 3) && (std::string_view{argv[3]} == "hang");

    const std::string start_count_path = startCountPath(g_identifier);
    g_start_count = read_start_count(start_count_path) + 1;
    write_to_file(start_count_path, g_start_count);
    write_to_file(pidPath(g_identifier), static_cast<long>(getpid()));

    // First start: reports running and stays running until killed by the control client.
    if (g_start_count == 1)
    {
        score::mw::lifecycle::report_running();

        // Expected to be killed by the control client. If it is stopped with SIGTERM instead, no report is
        // written, so the test fails on the missing report.
        TestRunner::WaitForTermination();
        return EXIT_FAILURE;
    }

    // Next `failed_restart_attempts` starts: terminate before reporting running.
    if (g_start_count <= 1 + g_failed_restart_attempts)
    {
        std::cout << g_identifier << " failing restart attempt " << (g_start_count - 1) << std::endl;
        std::_Exit(EXIT_FAILURE);
    }

    // Next start: If `hang` is specified, create the hanging file and wait for termination
    if (g_hang)
    {
        std::cout << g_identifier << " hanging in restart attempt " << (g_start_count - 1) << std::endl;
        static_cast<void>(touch_file(hangingPath(g_identifier)));
        TestRunner::WaitForTermination();
        return EXIT_FAILURE;
    }
    else
    {
        // Otherwise: report running, i.e. the restart succeeds.
        score::mw::lifecycle::report_running();
        std::cout << g_identifier << " restarted successfully" << std::endl;
        if (!touch_file(restartedPath(g_identifier)))
        {
            std::cerr << "Failed to create " << restartedPath(g_identifier) << std::endl;
        }
    }

    TestRunner::Init(g_identifier.c_str());
    const auto result = TestRunner::RunTests();
    TestRunner::WaitForTermination();
    return result;
}
