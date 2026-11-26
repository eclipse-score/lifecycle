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
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

#include "tests/integration/shared_dependency_shutdown/common.hpp"
#include "tests/utils/test_helper/arg_parser.hpp"
#include "tests/utils/test_helper/shm_counter.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{

/// @brief Program argument: how long the process keeps running after SIGTERM before it exits.
/// @details Models a component doing graceful shutdown work. Must stay below the configured
/// deployment_config.shutdown_timeout_ms, otherwise the Launch Manager escalates to SIGKILL.
struct StopDelayArg
{
    static constexpr std::string_view name = "--stop-delay-ms";
    using type = int;
    static constexpr type default_value = 0;
};

}  // namespace

std::string g_id;

TEST(SharedDependencyShutdown, DependentComponent)
{
    score::mw::lifecycle::test::ShmCounter start_counter{kStartCounterName};

    TEST_STEP("Take a start ticket")
    {
        static_cast<void>(++start_counter);
    }
    TEST_STEP("Report running")
    {
        score::mw::lifecycle::report_running();
    }
}

int main(int argc, char** argv)
{
    const char* const process_id = std::getenv("PROCESSIDENTIFIER");
    g_id = process_id != nullptr ? process_id : "unknown";

    const auto [stop_delay_ms] = score::mw::lifecycle::test::ParseArgs<StopDelayArg>(argc, argv);

    TestRunner::Init(("dependent_process_" + g_id).c_str());

    score::mw::lifecycle::test::ShmCounter stop_counter{kStopCounterName};

    const auto result = TestRunner::RunTests();
    TestRunner::WaitForTermination();

    // Graceful shutdown work. Until this process actually exits the Launch Manager must keep
    // treating it as running, and must not stop component_shared, which this component depends on.
    std::this_thread::sleep_for(std::chrono::milliseconds(stop_delay_ms));

    // Published immediately before exiting, so component_shared's SIGTERM handler has proof
    // this process was gone by the time it fires.
    static_cast<void>(++stop_counter);
    return result;
}
