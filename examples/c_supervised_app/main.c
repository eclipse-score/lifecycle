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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <score/mw/lifecycle/alive.h>
#include <score/mw/lifecycle/report_running.h>

static volatile sig_atomic_t exit_requested = 0;

static void signal_handler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        exit_requested = 1;
    }
}

int main(void)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    void* alive = score_lcm_alive_initialize("/example/instance");
    if (alive == NULL)
    {
        fprintf(stderr, "Failed to initialize alive instance\n");
        return EXIT_FAILURE;
    }

    if (score_mw_lifecycle_report_running() != 0)
    {
        fprintf(stderr, "Failed to report running state\n");
        score_lcm_alive_deinitialize(alive);
        return EXIT_FAILURE;
    }

    while (!exit_requested)
    {
        score_lcm_alive_report_alive(alive);
        usleep(50000);
    }

    score_lcm_alive_deinitialize(alive);
    return EXIT_SUCCESS;
}
