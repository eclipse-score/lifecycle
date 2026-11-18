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
#include <thread>

#include <score/mw/lifecycle/report_running.h>

namespace
{
volatile std::sig_atomic_t exit_requested{0};

void sigterm_handler(int signal)
{
    static_cast<void>(signal);
    exit_requested = 1;
}
}  // namespace

int main()
{
    if (std::signal(SIGTERM, sigterm_handler) == SIG_ERR)
    {
        std::cerr << "Failed to register SIGTERM handler" << std::endl;
        return EXIT_FAILURE;
    }

    score::mw::lifecycle::report_running();

    while (exit_requested == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return EXIT_SUCCESS;
}
