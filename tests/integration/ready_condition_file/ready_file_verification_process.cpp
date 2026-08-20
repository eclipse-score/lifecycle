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

#include "tests/utils/test_helper/test_helper.hpp"

// Given a correct configuration with:
//   - An initial Run Target named "Startup"
//   - Startup contains the Components "file_creating_component" and "verification_component"
//   - file_creating_component has a file_state ready condition on a file it only creates after a delay
//   - verification_component depends on file_creating_component

// When launch manager is started

std::string g_ready_file;

TEST(ReadyConditionFile, ReadyFileExistsBeforeDependentStarts)
{
    // Then, this process is only started once the ready condition of file_creating_component is met:
    TEST_STEP("Check that the ready condition file exists")
    {
        EXPECT_TRUE(std::filesystem::exists(g_ready_file))
            << "'" << g_ready_file << "' does not exist, the dependent component was started too early";
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Expected the path of the ready condition file as the only argument" << std::endl;
        return EXIT_FAILURE;
    }
    g_ready_file = argv[1];

    TestRunner runner{__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd};
    return runner.RunTests();
}
