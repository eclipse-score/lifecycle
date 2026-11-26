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

#include "tests/integration/shared_dependency_shutdown/common.hpp"
#include "tests/utils/test_helper/shm_counter.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{

score::mw::lifecycle::test::ShmCounter* g_stop_counter = nullptr;
score::mw::lifecycle::test::ShmCounter* g_violation_counter = nullptr;

/// @brief Records a violation if this process is asked to stop before both of its dependents
/// (component_fast, component_slow) have fully exited.
/// @details Async-signal-safe: both counters are lock-free atomics in shared memory.
void sigtermHandler(int)
{
    if (g_stop_counter->get() != kSharedDependentCount)
    {
        // Asked to stop while at least one of our dependents had not exited yet.
        static_cast<void>(++(*g_violation_counter));
    }
    TestRunner::g_exit_requested = true;
}

}  // namespace

TEST(SharedDependencyShutdown, ComponentShared)
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

int main()
{
    TestRunner::Init();

    // Map the counters and install the handler before running, so that a SIGTERM arriving at any
    // point after startup is accounted for.
    score::mw::lifecycle::test::ShmCounter stop_counter{kStopCounterName};
    score::mw::lifecycle::test::ShmCounter violation_counter{kViolationCounterName};
    g_stop_counter = &stop_counter;
    g_violation_counter = &violation_counter;
    std::signal(SIGTERM, sigtermHandler);

    const auto result = TestRunner::RunTests();
    TestRunner::WaitForTermination();

    // No graceful shutdown work of its own: component_shared's whole purpose in this test is to
    // be asked to stop at the right moment, not to take a while to stop itself.
    static_cast<void>(++stop_counter);
    return result;
}
