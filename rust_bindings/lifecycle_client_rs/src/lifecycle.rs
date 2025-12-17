/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
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

use libc::c_int;

#[link(name = "lifecycle_client")]
unsafe extern "C" {
    fn score_lcm_ReportExecutionStateRunning() -> c_int;
}

pub fn report_execution_state_running() -> bool {
    unsafe { score_lcm_ReportExecutionStateRunning() == 0 }
}
