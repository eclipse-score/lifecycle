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

#ifndef SCORE_MW_LAUNCH_MANAGER_WATCHDOG_WATCHDOG_FACTORY_HPP_INCLUDED
#define SCORE_MW_LAUNCH_MANAGER_WATCHDOG_WATCHDOG_FACTORY_HPP_INCLUDED

#include "score/mw/launch_manager/watchdog/i_watchdog.hpp"

#include <memory>

namespace score::mw::lifecycle::internal::watchdog
{

/// @brief Creates the watchdog used by the Launch Manager.
/// @details Defined by the implementation library selected with the `//config:watchdog_impl` label flag.
/// Must not have side effects on the watchdog hardware; arming happens in IWatchdog::init().
///
/// The full, unmodified Launch Manager command line is passed, so implementations can define their own options.
/// The Launch Manager ignores options it does not know. Constraints:
/// - `-c <config>` and `-h` are reserved by the Launch Manager. Implementations must skip them, e.g. by including
///   "c:h" in their getopt() optstring and ignoring these options.
/// - Implementations must ignore options they do not know.
///
/// @param argc Number of command-line arguments.
/// @param argv Command-line arguments.
/// @return The watchdog, or nullptr on failure, in which case the Launch Manager exits with a failure.
std::unique_ptr<IWatchdog> createWatchdog(int argc, const char* argv[]);

}  // namespace score::mw::lifecycle::internal::watchdog

#endif  // SCORE_MW_LAUNCH_MANAGER_WATCHDOG_WATCHDOG_FACTORY_HPP_INCLUDED
