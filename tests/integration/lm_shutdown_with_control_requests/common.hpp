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

#ifndef SCORE_TESTS_INTEGRATION_LM_SHUTDOWN_WITH_CONTROL_REQUESTS_COMMON_HPP
#define SCORE_TESTS_INTEGRATION_LM_SHUTDOWN_WITH_CONTROL_REQUESTS_COMMON_HPP

#include <string_view>

/// @brief Written by component_a when it has reported running (run_target_a is
/// active).
constexpr std::string_view a_started = "component_a_started";

/// @brief Written by component_a when it starts being terminated
constexpr std::string_view a_terminating = "component_a_terminating";

#endif  // SCORE_TESTS_INTEGRATION_LM_SHUTDOWN_WITH_CONTROL_REQUESTS_COMMON_HPP
