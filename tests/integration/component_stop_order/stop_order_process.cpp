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
#include <optional>
#include <string>

#include "tests/utils/test_helper/shm_counter.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{
constexpr std::string_view kCounterName = "/component_stop_order_counter";

score::mw::lifecycle::test::ShmCounter* g_counter = nullptr;
int g_expected_position = 0;

void sigtermHandler(int)
{
    // only increment if the order is correct, test shall not pass if the end
    // number != number of components
    g_counter->increment_if_equals(g_expected_position - 1);
    TestRunner::g_exit_requested = true;
}
}  // namespace

TEST(ComponentStopOrder, Component)
{
    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }
}

int main(int argc, char** argv)
{
    const char* process_id = std::getenv("PROCESSIDENTIFIER");
    const std::string id = process_id ? process_id : "unknown";
    g_expected_position = argc > 1 ? std::atoi(argv[1]) : 0;
    TestRunner::Init(("stop_order_process_" + id).c_str());

    // init the shm
    score::mw::lifecycle::test::ShmCounter counter{kCounterName};
    g_counter = &counter;

    std::signal(SIGTERM, sigtermHandler);

    const auto result = TestRunner::RunTests();
    TestRunner::WaitForTermination();
    return result;
}
