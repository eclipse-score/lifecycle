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
#ifndef TESTS_INTEGRATION_COMPONENT_RESTART_COMPONENT_RESTART_FILES_HPP
#define TESTS_INTEGRATION_COMPONENT_RESTART_COMPONENT_RESTART_FILES_HPP

#include <string>
#include <string_view>

/// @brief Created by the control client once it has verified the recovery. The dependent component checks
/// that it kept running at least until then.
constexpr std::string_view kRecoveryVerifiedFile = "recovery_verified";

/// @brief Created by the dependent component when it starts; a second start fails the test.
constexpr std::string_view kDependentStartedFile = "dependent_started";

/// @brief Identifiers of the components configured with the restart recovery action.
constexpr std::string_view kCrashingA = "crashing_a";
constexpr std::string_view kCrashingB = "crashing_b";
constexpr std::string_view kCrashingC = "crashing_c";

/// @return File storing how often the crashing component `component` has been started so far.
inline std::string startCountPath(const std::string_view component)
{
    return std::string{component} + "_start_count";
}

/// @return File in which the crashing component `component` stores its pid on every start.
inline std::string pidPath(const std::string_view component)
{
    return std::string{component} + ".pid";
}

/// @return File the crashing component `component` creates when a restart attempt started in "hang" mode,
/// i.e. it will not report running.
inline std::string hangingPath(const std::string_view component)
{
    return std::string{component} + "_hanging";
}

/// @return File the crashing component `component` creates once it has been restarted successfully.
inline std::string restartedPath(const std::string_view component)
{
    return std::string{component} + "_restarted";
}

#endif  // TESTS_INTEGRATION_COMPONENT_RESTART_COMPONENT_RESTART_FILES_HPP
