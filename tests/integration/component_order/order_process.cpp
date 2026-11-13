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
#include <tuple>

#include "tests/utils/test_helper/arg_parser.hpp"
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
    ASSERT_GT(g_expected_start_position, 0) << "Expected launch position must be given with --start-pos";

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

namespace
{
/// @brief Program argument for start position in launch order.
struct StartPosArg
{
    static constexpr std::string_view name = "--start-pos";
    using type = int;
    static constexpr type default_value = 0;
};

/// @brief Program argument for stop position in shutdown order.
struct StopPosArg
{
    static constexpr std::string_view name = "--stop-pos";
    using type = int;
    static constexpr type default_value = 0;
};
}  // namespace

int main(int argc, char** argv)
{
    const char* process_id = std::getenv("PROCESSIDENTIFIER");
    g_id = process_id ? process_id : "unknown";
    std::tie(g_expected_start_position, g_expected_stop_position) =
        score::mw::lifecycle::test::ParseArgs<StartPosArg, StopPosArg>(argc, argv);
    TestRunner::Init(("order_process_" + g_id).c_str());

    // init the stop shm and install the handler before running
    score::mw::lifecycle::test::ShmCounter stop_counter{kStopCounterName};
    g_stop_counter = &stop_counter;
    std::signal(SIGTERM, sigtermHandler);
    const auto result = TestRunner::RunTests();
    TestRunner::WaitForTermination();
    return result;
}
