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
#include <filesystem>
#include <string>
#include <string_view>

/// @return File path to an xml adjacent to the input file path
std::string xmlPath(const std::string_view file)
{
    return std::filesystem::path{file}.filename().stem().string() + ".xml";
}

/// @brief Creates an empty file.
/// @return AssertionSuccess if the file is correctly created.
inline testing::AssertionResult touch_file(const std::string_view file_path)
{
    auto openRes = fopen(file_path.data(), "w+");
    if (!openRes)
        return testing::AssertionFailure()
               << "Could not touch file " << file_path << " errno: " << errno << " message: " << strerror(errno);

    if (fclose(openRes) != 0)
        return testing::AssertionFailure()
               << "Couldn't close opened file " << file_path << " errno: " << errno << " message: " << strerror(errno);
    return testing::AssertionSuccess();
}

/// @brief Location to store a file signalling that the fallback state has been reached.
constexpr std::string_view fallback_file = "fallback_reached";

/// @brief Location to store a file signalling that a process has been killed on first try - used to test ready recovery action
constexpr std::string_view crashed_once_file = "crashed_once";

/// @brief Location to store a file signalling that a process has been killed on second try - used to test ready recovery action
constexpr std::string_view crashed_twice_file = "crashed_twice";

/// @brief Where to store the test_end signal file. This must be kept consistent with where the test framework
/// searches for files.
constexpr std::string_view test_end_location = "../test_end";

/// @brief Call at the start of a test to check for leftover files from a previous run
/// Files can be leftover when running manually on the host system, but otherwise are cleaned up
/// by the test framework.
/// @param[in] files Files to check
/// @param[in] strict If true, return a failure if any files exist. Otherwise attempt to remove them.
[[nodiscard]]
inline testing::AssertionResult check_clean(const std::initializer_list<std::string_view> files,
                                            const bool strict = true)
{
    std::stringstream failures{};
    for (const auto file : files)
    {
        if (!std::filesystem::exists(file))
        {
            continue;
        }

        if (strict)
        {
            failures << "'" << file << "' already exists!\n";
        }
        else if (!std::filesystem::remove(file))
        {
            failures << "Failed to remove '" << file << "'!\n";
        }
    }
    if (failures.tellp() > 0)
    {
        return testing::AssertionFailure() << failures.str();
    }
    return testing::AssertionSuccess();
}

#define TEST_STEP(message)                                                                 \
    for (bool once = (std::cout << "[ STEP     ] " << (message) << std::endl, true); once; \
         (std::cout << "[ END STEP ] " << (message) << std::endl), once = false)

/// @brief Helper class to setup, run, and clean up GTEST tests
class TestRunner
{
    static void signalHandler(int)
    {
        exitRequested = true;
    }

    bool signal_completion;
    bool m_wait_for_termination;

    std::string_view m_test_path;

  public:
    /// @brief TestRunner constructor
    /// @param[in] test_path location to write the GTEST xml file (usually __FILE__)
    /// @param[in] wait_for_termination whether to block on RunTests() until termination is requested
    /// @param[in] do_signal_completion whether this test should deploy a file signaling the test has completed
    ///            Usually the control daemon should deploy this file.
    TestRunner(std::string_view test_path, bool wait_for_termination = true, bool do_signal_completion = false)
    {
        m_test_path = test_path;
        m_wait_for_termination = wait_for_termination;

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        signal_completion = do_signal_completion;
    }

    ~TestRunner()
    {
        if (m_wait_for_termination && !exitRequested)
        {
            pause();
        }

        if (signal_completion)
        {
            const auto res = touch_file(test_end_location);
            if (!res) {
                std::cerr << res.failure_message() << std::endl;
            }
        }
    }

    /// @brief True if the test process has received SIGINT or SIGTERM
    inline static std::atomic<bool> exitRequested = false;

    /// @brief Use this function in main() to run all tests. It returns 0 if all tests are successful, or 1 otherwise.
    int RunTests()
    {
        ::testing::GTEST_FLAG(output) = "xml:" + xmlPath(m_test_path);
        testing::InitGoogleTest();

        auto res = RUN_ALL_TESTS();

        return res;
    }
};
