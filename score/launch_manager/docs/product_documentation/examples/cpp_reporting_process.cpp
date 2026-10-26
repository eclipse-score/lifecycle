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

#include <unistd.h>
#include <atomic>
#include <cassert>
#include <csignal>
#include <cstdlib>
#include <iostream>

#include <score/mw/lifecycle/report_running.h>

volatile std::sig_atomic_t exitRequested{0};

void sigTermHandler(int)
{
    exitRequested = 1;
}

int main()
{

    // register signal handler so that lcm can request a termination
    assert(signal(SIGTERM, sigTermHandler));

    // report running state
    score::mw::lifecycle::report_running();

    // do some work

    // wait until lm wants to terminate
    while (exitRequested == 0)
    {
        pause();
    }

    return EXIT_SUCCESS;
}
