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

#include "tests/integration/shared_dependency_shutdown/common.hpp"
#include "tests/utils/test_helper/arg_parser.hpp"
#include "tests/utils/test_helper/shm_counter.hpp"
#include "tests/utils/test_helper/test_runner.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{

/// @brief Program argument: this instance is the dependent that terminates first.
/// @details Exactly one of the two dependents must be started with it. The other one stays alive
/// past that point, so that there is always a dependent still shutting down when the first one
/// is gone - the situation in which component_shared must not be stopped.
struct ExitFirstArg
{
    static constexpr std::string_view name = "--exit-first";
    using type = bool;
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

    const auto [exit_first] = score::mw::lifecycle::test::ParseArgs<ExitFirstArg>(argc, argv);

    TestRunner::Init(("dependent_process_" + g_id).c_str());

    score::mw::lifecycle::test::ShmCounter stop_counter{kStopCounterName};
    score::mw::lifecycle::test::ShmCounter deactivating_counter{kDeactivatingCounterName};
    score::mw::lifecycle::test::ShmCounter violation_counter{kViolationCounterName};
    score::mw::lifecycle::test::ShmCounter sync_failure_counter{kSyncFailureCounterName};

    const auto result = TestRunner::RunTests();
    TestRunner::WaitForTermination();

    // Everything below models graceful shutdown work: until this process actually exits the
    // Launch Manager must keep treating it as running, and must not stop component_shared, which
    // this component depends on.

    // Announce that this dependent's graceful shutdown has begun, then block until the other one
    // has announced the same. Afterwards both dependents are provably shutting down at the same
    // time, which is the precondition the rest of the test needs. A timeout here means the Launch
    // Manager did not deactivate the two dependents concurrently.
    static_cast<void>(++deactivating_counter);
    const bool both_deactivating = waitUntil(
        [&deactivating_counter] {
            return deactivating_counter.get() == kSharedDependentCount;
        },
        kHandshakeTimeout);
    if (!both_deactivating)
    {
        static_cast<void>(++sync_failure_counter);
    }

    if (!exit_first)
    {
        // Stay alive until the other dependent has fully terminated. That is the moment the
        // Launch Manager learns about the first dependent's termination, and the moment at which
        // it used to release component_shared even though this process is still running.
        // component_shared publishes a stop ticket too, but only after it has been signalled -
        // which, this early, is itself the failure this test looks for and is recorded as a
        // violation before the ticket appears. So either way the wait below ends on the event
        // the observation window needs.
        const bool other_exited = waitUntil(
            [&stop_counter] {
                return stop_counter.get() >= 1;
            },
            kHandshakeTimeout);
        if (!other_exited)
        {
            static_cast<void>(++sync_failure_counter);
        }

        // Hold that state open long enough for a premature stop of component_shared to be
        // observed, and no longer: the wait ends as soon as component_shared reports one.
        static_cast<void>(waitUntil(
            [&violation_counter] {
                return violation_counter.get() > 0;
            },
            kPrematureStopObservationWindow));
    }

    // Published immediately before exiting, so component_shared's SIGTERM handler has proof
    // this process was gone by the time it fires.
    static_cast<void>(++stop_counter);
    return result;
}
