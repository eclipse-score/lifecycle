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
#include <filesystem>
#include <string>
#include <string_view>
#include <gtest/gtest.h>

/// @return File path to an xml adjacent to the input file path
std::string xmlPath(const std::string_view file) {
    return std::filesystem::path{file}.filename().stem().string() + ".xml";
}

/// @brief Creates an empty file.
/// @return AssertionSuccess if the file is correctly created.
inline testing::AssertionResult touch_file(const std::string_view file_path) {
    auto openRes = fopen(file_path.data(), "w+");
    if (!openRes)
        return testing::AssertionFailure()
               << "Could not touch file " << file_path << " errno: " << errno << " message: " << strerror(errno);

    if (fclose(openRes) != 0)
        return testing::AssertionFailure()
               << "Couldn't close opened file " << file_path << " errno: " << errno << " message: " << strerror(errno);
    return testing::AssertionSuccess();
}

#define TEST_STEP(message)                                              \
    for (bool once =                                                    \
            (std::cout << "[ STEP     ] " << (message) << std::endl,    \
            true);                                                      \
    once;                                                               \
    (std::cout << "[ END STEP ] " << (message) << std::endl),           \
    once = false)


/// @brief Helper class to setup, run, and clean up GTEST tests
class TestRunner {
    inline static std::atomic<bool> exitRequested = false;

    static void signalHandler(int) {
        exitRequested = true;
    }

    bool signal_completion;

public:
    /// @brief TestRunner constructor
    /// @param[in] test_path location to write the GTEST xml file (usually __FILE__)
    /// @param[in] do_signal_completion whether this test should deploy a file signaling the test has completed
    ///            Usually the control daemon should deploy this file.
    TestRunner(std::string test_path, bool do_signal_completion=false) {
        ::testing::GTEST_FLAG(output) = "xml:" + xmlPath(test_path);
        testing::InitGoogleTest();

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        signal_completion = do_signal_completion;
    }

    ~TestRunner() {
        if (!exitRequested) {
            pause();
        }

        if (signal_completion) {
            static_cast<void>(touch_file("../test_end"));
        }
    }

    /// @brief Use this function in main() to run all tests. It returns 0 if all tests are successful, or 1 otherwise.
    int RunTests() {
        auto res = RUN_ALL_TESTS();

        return res;
    }
};