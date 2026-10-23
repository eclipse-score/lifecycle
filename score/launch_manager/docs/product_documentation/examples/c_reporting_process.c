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
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "score/mw/lifecycle/report_running.h"

const struct timespec interval = {
    .tv_sec = 0,
    .tv_nsec = 50000000,
};

volatile sig_atomic_t exitRequested = 0;

static void sigTermHandler(int signal)
{
    (void)signal;
    exitRequested = 1;
}

int main(void)
{
    // register SIGTERM handler (not inside assert, as it would be removed with NDEBUG)
    if(signal(SIGTERM, sigTermHandler) == SIG_ERR){
        (void) printf("Failed to register SIGTERM handler! Terminating");
        return EXIT_FAILURE;
    }

    // report running
    int8_t result = score_mw_lifecycle_report_running();

    // check report was successful
    if(result != 0){
        printf("Failed to report kRunning to LCM! Terminating");
        return 1;
    }

    // wait until lm wants to terminate
    while(exitRequested == 0){
        nanosleep(&interval, NULL);
    }

    return 0;
}
