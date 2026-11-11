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
#include <csignal>
#include <cstdlib>
#include <string>

#include "tests/utils/test_helper/shm_counter.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{
constexpr std::string_view kStartCounterName = "/component_start_order_counter";
constexpr std::string_view kStopCounterName = "/component_stop_order_counter";

score::mw::lifecycle::test::ShmCounter* g_stop_counter = nullptr;
int g_expected_stop_position = 0;

void sigtermHandler(int)
{
    // only increment if the order is correct, test shall not pass if the end
    // number != number of components
    g_stop_counter->increment_if_equals(g_expected_stop_position - 1);
    TestRunner::g_exit_requested = true;
}
}  // namespace

std::string g_id;
int g_expected_start_position = 0;

TEST(ComponentOrder, Component)
{
    ASSERT_GT(g_expected_start_position, 0) << "Expected launch position must be given as the first argument";

    score::mw::lifecycle::test::ShmCounter counter{kStartCounterName};
    int order = 0;

    TEST_STEP("Get the order")
    {
        order = ++counter;
    }
    TEST_STEP("Check order")
    {
        EXPECT_EQ(order, g_expected_start_position) << "Component " << g_id << " was launched out of order";
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
    g_expected_start_position = argc > 1 ? std::atoi(argv[1]) : 0;
    g_expected_stop_position = argc > 2 ? std::atoi(argv[2]) : 0;
    TestRunner::Init(("order_process_" + g_id).c_str());

    // init the stop shm and install the handler before running
    score::mw::lifecycle::test::ShmCounter stop_counter{kStopCounterName};
    g_stop_counter = &stop_counter;
    std::signal(SIGTERM, sigtermHandler);
    const auto result = TestRunner::RunTests();
    TestRunner::WaitForTermination();
    return result;
}
