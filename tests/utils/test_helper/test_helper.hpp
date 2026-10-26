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
#ifndef TESTS_UTILS_TEST_HELPER_HPP
#define TESTS_UTILS_TEST_HELPER_HPP

#include <gtest/gtest.h>
#include <chrono>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>

#include <score/mw/lifecycle/ilm_control.hpp>

/// @brief Creates an empty file.
/// @return AssertionSuccess if the file is correctly created.
testing::AssertionResult touch_file(std::string_view file_path);

/// @brief Location to store a file signalling that the fallback state has been reached.
constexpr std::string_view fallback_file = "fallback_reached";

/// @brief Prefix of the file storing the number of times a process has crashed so far - used to test ready
/// recovery action
constexpr std::string_view crash_count_file = "crash_count";

/// @return File path to store the crash count for a process configured to crash `crashes_until_success` times
inline std::string crashCountPath(const int crashes_until_success)
{
    return std::string{crash_count_file} + "_" + std::to_string(crashes_until_success);
}

/// @brief Call at the start of a test to check for leftover files from a previous run
/// Files can be leftover when running manually on the host system, but otherwise are cleaned up
/// by the test framework.
/// @param[in] files Files to check
/// @param[in] strict If true, return a failure if any files exist. Otherwise attempt to remove them.
[[nodiscard]]
testing::AssertionResult check_clean(std::initializer_list<std::string_view> files, bool strict = true);

/// @brief Waits until all of `files` exist. The timeout applies to all files together, not to each one.
/// @return AssertionSuccess if every file appeared within the timeout, otherwise a failure naming the missing files.
[[nodiscard]]
testing::AssertionResult wait_for_files(
    std::initializer_list<std::string_view> files,
    std::chrono::milliseconds timeout = std::chrono::seconds(3));

/// @brief Waits until `file` exists.
/// @return AssertionSuccess if the file appeared within the timeout.
[[nodiscard]]
testing::AssertionResult wait_for_file(
    std::string_view file,
    std::chrono::milliseconds timeout = std::chrono::seconds(3));

using score::mw::lifecycle::RunTargetActivationSource;
using score::mw::lifecycle::RunTargetName;

/// @brief Hands one activation event over from the activation callback to the test thread (single-slot queue).
/// Blocks while the previous event has not been popped yet.
void push_event(RunTargetActivationSource source, RunTargetName target);

/// @brief Blocks until an event is available, then passes it to `callback` and frees the slot.
/// If no event arrives within `timeout`, a non-fatal test failure is recorded and `callback` is not invoked.
void pop_event(
    std::function<void(RunTargetActivationSource source, RunTargetName target)> callback,
    std::chrono::milliseconds timeout = std::chrono::seconds(10));

#endif  // TESTS_UTILS_TEST_HELPER_HPP
