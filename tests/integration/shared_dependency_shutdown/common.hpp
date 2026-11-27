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
#ifndef SCORE_TESTS_INTEGRATION_SHARED_DEPENDENCY_SHUTDOWN_COMMON_HPP
#define SCORE_TESTS_INTEGRATION_SHARED_DEPENDENCY_SHUTDOWN_COMMON_HPP

#include <chrono>
#include <string_view>
#include <thread>

/// @brief Incremented once by every component process as it starts up.
/// @details Shared between component_shared_process (component_shared) and dependent_process
/// (component_fast, component_slow).
constexpr std::string_view kStartCounterName = "/shared_dependency_start_counter";

/// @brief Incremented once by every component process immediately before it exits, i.e. after
/// its graceful shutdown work is done. Reading it tells a process how many of the other
/// components have already fully terminated.
/// @details Shared between component_shared_process (component_shared) and dependent_process
/// (component_fast, component_slow).
constexpr std::string_view kStopCounterName = "/shared_dependency_stop_counter";

/// @brief Incremented once by every dependent of component_shared as soon as it has observed
/// its own SIGTERM, i.e. when its graceful shutdown begins.
/// @details The dependents use this to rendezvous: each of them waits until this counter equals
/// kSharedDependentCount before going on, so both are provably inside their graceful shutdown at
/// the same time. Without that handshake the test would merely assume the deactivations overlap.
constexpr std::string_view kDeactivatingCounterName = "/shared_dependency_deactivating_counter";

/// @brief Incremented by component_shared_process when it is asked to stop while the stop
/// counter does not yet equal kSharedDependentCount, i.e. while one of its dependents was still
/// shutting down. Must be zero at the end of the test.
constexpr std::string_view kViolationCounterName = "/shared_dependency_violation_counter";

/// @brief Incremented by a dependent whenever one of its bounded waits times out.
/// @details The handshakes below run after the process' GTEST assertions have already been
/// written out, so a broken handshake cannot be reported as a test failure by the process
/// itself. It is reported through this counter instead, which control_client_test_driver
/// asserts to be zero: a test that stopped synchronizing must fail loudly rather than silently
/// degrade into the timing-based test it replaced.
constexpr std::string_view kSyncFailureCounterName = "/shared_dependency_sync_failure_counter";

/// @brief Number of component processes started by run_target_diamond
/// (component_shared, component_fast, component_slow).
constexpr int kComponentCount = 3;

/// @brief Number of dependents of component_shared (component_fast, component_slow). Both must
/// have fully exited before component_shared may be asked to stop. Read directly by
/// component_shared_process's SIGTERM handler.
constexpr int kSharedDependentCount = 2;

/// @brief Upper bound for each of the dependents' rendezvous waits.
/// @details Only reached if the expected event never happens, in which case the wait reports a
/// sync failure and the test fails. The sum of all waits a single dependent can perform must
/// stay below the configured deployment_config.shutdown_timeout_ms, otherwise the Launch Manager
/// escalates to SIGKILL and the component never gets to publish its result.
constexpr std::chrono::milliseconds kHandshakeTimeout{1000};

/// @brief How long component_slow stays alive after it has seen component_fast exit.
/// @details This is the window in which a premature stop of component_shared is detected: one of
/// its dependents has fully terminated while the other (component_slow) demonstrably has not.
/// Before the fix the Launch Manager signalled component_shared within this window; the wait ends
/// early as soon as that is observed, so the window only costs wall-clock time when the test
/// passes.
constexpr std::chrono::milliseconds kPrematureStopObservationWindow{1000};

/// @brief Polling interval of waitUntil().
constexpr std::chrono::milliseconds kPollInterval{5};

/// @brief Polls @p predicate until it returns true or @p timeout elapses.
/// @return True if the predicate became true, false on timeout.
template <typename Predicate>
bool waitUntil(Predicate predicate, const std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!predicate())
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        std::this_thread::sleep_for(kPollInterval);
    }
    return true;
}

#endif  // SCORE_TESTS_INTEGRATION_SHARED_DEPENDENCY_SHUTDOWN_COMMON_HPP
