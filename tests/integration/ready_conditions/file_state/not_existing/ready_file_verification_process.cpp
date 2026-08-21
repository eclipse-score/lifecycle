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

#include "tests/utils/test_helper/test_helper.hpp"

std::string g_ready_file;
bool g_expect_existing = true;

TEST(ReadyConditionFile, ReadyConditionIsMetBeforeDependentStarts)
{
    // Then, this process is only started once the ready condition of the component it depends on is met:
    TEST_STEP("Check the state of the ready condition file")
    {
        if (g_expect_existing)
        {
            EXPECT_TRUE(std::filesystem::exists(g_ready_file))
                << "'" << g_ready_file << "' does not exist, the dependent component was started too early";
        }
        else
        {
            EXPECT_FALSE(std::filesystem::exists(g_ready_file))
                << "'" << g_ready_file << "' still exists, the dependent component was started too early";
        }
    }
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Expected the path of the ready condition file and its expected state "
                     "('Exists' or 'NotExisting') as arguments"
                  << std::endl;
        return EXIT_FAILURE;
    }
    g_ready_file = argv[1];

    const std::string_view expected_state{argv[2]};
    if (expected_state == "Exists")
    {
        g_expect_existing = true;
    }
    else if (expected_state == "NotExisting")
    {
        g_expect_existing = false;
    }
    else
    {
        std::cerr << "Unknown expected state '" << expected_state << "', expected 'Exists' or 'NotExisting'"
                  << std::endl;
        return EXIT_FAILURE;
    }

    TestRunner runner{__FILE__, TerminationBehavior::kContinue, TerminationNotification::kTestEnd};
    return runner.RunTests();
}
