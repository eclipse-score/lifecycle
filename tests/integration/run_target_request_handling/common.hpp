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

#ifndef SCORE_TESTS_INTEGRATION_RUN_TARGET_REQUEST_HANDLING_COMMON_HPP
#define SCORE_TESTS_INTEGRATION_RUN_TARGET_REQUEST_HANDLING_COMMON_HPP

#include <string>
#include <string_view>

/// @brief Components deploying the gated_process binary. Each one only belongs
/// to the Run Target of the same suffix.
constexpr std::string_view gated_a = "gated_a";
constexpr std::string_view gated_b = "gated_b";

/// @return File the control client creates to let `component` report running.
/// Until it exists, the activation of the component's Run Target stays in
/// progress, which gives the control client a deterministic window to send
/// further requests.
inline std::string release_file(const std::string_view component)
{
    return std::string{component} + "_release";
}

/// @return File `component` creates as soon as it has been started, before it
/// waits for its release file.
inline std::string started_file(const std::string_view component)
{
    return std::string{component} + "_started";
}

/// @return File `component` creates right after it has reported running.
inline std::string running_file(const std::string_view component)
{
    return std::string{component} + "_running";
}

#endif  // SCORE_TESTS_INTEGRATION_RUN_TARGET_REQUEST_HANDLING_COMMON_HPP
