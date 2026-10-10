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
#ifndef TESTS_UTILS_TEST_HELPER_GATED_PROCESS_HPP
#define TESTS_UTILS_TEST_HELPER_GATED_PROCESS_HPP

#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <thread>

#include <score/mw/lifecycle/ilm_control.hpp>

/// @return File the gated_process deployed as `component` creates when it starts.
inline std::string gatedStartedPath(const std::string_view component)
{
    return std::string{component} + "_started";
}

/// @return File that lets the gated_process deployed as `component` report running.
inline std::string gatedReleasePath(const std::string_view component)
{
    return std::string{component} + "_release";
}

/// @brief Waits until `file` exists.
/// @return AssertionSuccess if the file appeared within the timeout.
[[nodiscard]]
inline testing::AssertionResult wait_for_file(
    const std::string_view file,
    const std::chrono::milliseconds timeout = std::chrono::seconds(3))
{
    constexpr auto kPollInterval = std::chrono::milliseconds(10);
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (!std::filesystem::exists(file))
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return testing::AssertionFailure() << "'" << file << "' was not created";
        }
        std::this_thread::sleep_for(kPollInterval);
    }
    return testing::AssertionSuccess();
}

/// @brief Waits until the launch manager reports an activation in progress.
///
/// An accepted request is only recorded as pending; its transition starts on a later
/// main loop cycle. Wait for this before sending a request that must arrive during it.
/// @return AssertionSuccess if get_active_run_target() returned kActivationInProgress
///         within the timeout.
[[nodiscard]]
inline testing::AssertionResult wait_for_activation_in_progress(
    score::mw::lifecycle::ILmControl& client,
    const std::chrono::milliseconds timeout = std::chrono::seconds(3))
{
    constexpr auto kPollInterval = std::chrono::milliseconds(10);
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true)
    {
        const auto result = client.get_active_run_target();
        if (!result.has_value() && (result.error() == score::mw::lifecycle::ExecErrc::kActivationInProgress))
        {
            return testing::AssertionSuccess();
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return testing::AssertionFailure() << "Activation did not reach the in-progress state";
        }
        std::this_thread::sleep_for(kPollInterval);
    }
}

#endif  // TESTS_UTILS_TEST_HELPER_GATED_PROCESS_HPP
