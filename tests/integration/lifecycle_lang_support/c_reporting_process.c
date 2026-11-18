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

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <score/mw/lifecycle/report_running.h>

static volatile sig_atomic_t exit_requested = 0;

static void sigterm_handler(int signum)
{
    (void)signum;
    exit_requested = 1;
}

int main(void)
{
    if (signal(SIGTERM, sigterm_handler) == SIG_ERR)
    {
        (void)fprintf(stderr, "Failed to register SIGTERM handler\n");
        return EXIT_FAILURE;
    }

    if (score_mw_lifecycle_report_running() != 0)
    {
        (void)fprintf(stderr, "Failed to report running\n");
        return EXIT_FAILURE;
    }

    while (exit_requested == 0)
    {
        pause();
    }
    return EXIT_SUCCESS;
}
