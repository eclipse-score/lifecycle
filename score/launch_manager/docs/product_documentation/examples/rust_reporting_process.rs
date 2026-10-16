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

use signal_hook::consts::SIGTERM;
use signal_hook::iterator::Signals;

fn main() -> Result<(), &'static str> {
    // register signal handler so that lcm can request a termination
    let mut signals =
        Signals::new([SIGTERM]).map_err(|_| "Failed to register signal handler")?;

    // report running and check result
    if !lifecycle_client_rs::report_running() {
        return Err("Failed to report execution state");
    }

    // do work

    // wait until lm wants to terminate
    signals.forever().next();

    Ok(())
}
