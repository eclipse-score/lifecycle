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

#include <string_view>

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

/// @brief Incremented by component_shared_process when it is asked to stop while the stop
/// counter does not yet equal kSharedDependentCount, i.e. while one of its dependents was still
/// shutting down. Must be zero at the end of the test.
constexpr std::string_view kViolationCounterName = "/shared_dependency_violation_counter";

/// @brief Number of component processes started by run_target_diamond
/// (component_shared, component_fast, component_slow).
constexpr int kComponentCount = 3;

/// @brief Number of dependents of component_shared (component_fast, component_slow). Both must
/// have fully exited before component_shared may be asked to stop. Read directly by
/// component_shared_process's SIGTERM handler.
constexpr int kSharedDependentCount = 2;

#endif  // SCORE_TESTS_INTEGRATION_SHARED_DEPENDENCY_SHUTDOWN_COMMON_HPP
