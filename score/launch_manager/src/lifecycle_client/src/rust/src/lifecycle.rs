// *******************************************************************************
// Copyright (c) 2026 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************
#[link(name = "report_running")]
unsafe extern "C" {
    fn score_mw_lifecycle_report_running() -> i8;
}

pub fn report_running() -> bool {
    unsafe { score_mw_lifecycle_report_running() == 0 }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn given_no_ipc_when_report_running_called_the_returns_false() {
        // Given no launch manager IPC setup
        // When report_running() is called
        // Then report_running() returns false
        assert!(!report_running());
    }
}
