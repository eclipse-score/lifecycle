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

#include "score/mw/lifecycle/report_running.h"
#include <unistd.h>
#include <stdio.h>

int main(int argc, char* argv[]){

    // report running
    int8_t result = score_mw_lifecycle_report_running();

    // check report was successful
    if(result != 0){
        printf("Failed to report kRunning to LCM! Terminating");
        return 1;
    }

    return 0;
}
