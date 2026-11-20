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

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <thread>

#include "tests/utils/test_helper/arg_parser.hpp"
#include <score/mw/lifecycle/alive.h>
#include <score/mw/lifecycle/report_running.h>

namespace
{
using score::mw::lifecycle::test::ParseArgs;

struct Reports
{
    static constexpr std::string_view name = "--reports";
    using type = int;
    static constexpr type default_value = -1;  // negative: unlimited
};

struct PeriodMs
{
    static constexpr std::string_view name = "--period_ms";
    using type = int;
    static constexpr type default_value = 50;
};

volatile std::sig_atomic_t exit_requested{0};

void sigterm_handler(int signal)
{
    static_cast<void>(signal);
    exit_requested = 1;
}
}  // namespace

/// @brief Reports running, then reports alive every 50ms.
///        optional argument "--reports=<n>": only the first <n> alive
///        notifications are reported.
///        optional argument "--period_ms=<ms>": time between two alive
///        notifications (default 50).
int main(int argc, char** argv)
{
    auto [remaining_reports, period_ms] = ParseArgs<Reports, PeriodMs>(argc, argv);

    if (std::signal(SIGTERM, sigterm_handler) == SIG_ERR)
    {
        std::cerr << "Failed to register SIGTERM handler" << std::endl;
        return EXIT_FAILURE;
    }

    score::mw::lifecycle::Alive alive{"cpp_alive_process"};
    score::mw::lifecycle::report_running();

    while (exit_requested == 0)
    {
        if (remaining_reports != 0)
        {
            alive.ReportAlive();
            if (remaining_reports > 0)
            {
                --remaining_reports;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(period_ms));
    }
    return EXIT_SUCCESS;
}
