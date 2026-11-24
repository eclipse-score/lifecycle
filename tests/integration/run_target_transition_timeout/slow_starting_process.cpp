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

#include <score/mw/lifecycle/report_running.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

/// @file  slow_starting_process.cpp
/// @brief Test process that waits for a configurable number of milliseconds before
///        reporting running.
/// @details The wait is longer than the transition timeout of the Run Target that
///          depends on this process, but well inside the process' own
///          `ready_timeout_ms`. The Run Target activation therefore fails on the
///          transition timeout rather than on the component's ready timeout, which
///          is what run_target_transition_timeout.py sets out to verify. The Launch
///          Manager terminates this process while it is still waiting, so it never
///          reports running.

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " <delay in milliseconds>\n";
        return EXIT_FAILURE;
    }

    const long delay_ms = std::strtol(argv[1], nullptr, 10);
    if (delay_ms <= 0)
    {
        std::cerr << "the delay must be a positive number of milliseconds, got '" << argv[1] << "'\n";
        return EXIT_FAILURE;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{delay_ms});

    // Not reached while the configured transition timeout is shorter than the delay:
    // the process is terminated during the recovery transition. Reporting running here
    // keeps the process usable for a configuration in which the delay does fit.
    score::mw::lifecycle::report_running();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds{1});
    }
}
