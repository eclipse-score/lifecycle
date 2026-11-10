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
#include <cstdlib>
#include <string>

#include "tests/utils/test_helper/shm_counter.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{
constexpr std::string_view kCounterName = "/component_start_order_counter";
}  // namespace

std::string g_id;
int g_expected_position = 0;

TEST(ComponentStartOrder, Component)
{
    ASSERT_GT(g_expected_position, 0) << "Expected launch position must be given as the first argument";

    score::mw::lifecycle::test::ShmCounter counter{kCounterName};
    int order = 0;

    TEST_STEP("Get the order")
    {
        order = ++counter;
    }
    TEST_STEP("Chek order")
    {
        EXPECT_EQ(order, g_expected_position) << "Component " << g_id << " was launched out of order";
    }
    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }
}

int main(int argc, char** argv)
{
    const char* process_id = std::getenv("PROCESSIDENTIFIER");
    g_id = process_id ? process_id : "unknown";
    g_expected_position = argc > 1 ? std::atoi(argv[1]) : 0;
    TestRunner::Init(("start_order_process_" + g_id).c_str());
    const auto result = TestRunner::RunTests();
    TestRunner::WaitForTermination();
    return result;
}
