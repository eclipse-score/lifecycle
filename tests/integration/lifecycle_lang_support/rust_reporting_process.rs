// *******************************************************************************
// Copyright (c) 2026 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

use std::sync::atomic::{AtomicBool, Ordering};

const SIGTERM: i32 = 15;

static STOP: AtomicBool = AtomicBool::new(false);

unsafe extern "C" {
    fn signal(signum: i32, handler: extern "C" fn(i32)) -> usize;
}

extern "C" fn sigterm_handler(_: i32) {
    STOP.store(true, Ordering::Relaxed);
}

fn main() -> Result<(), &'static str> {
    unsafe {
        signal(SIGTERM, sigterm_handler);
    }

    if !lifecycle_client_rs::report_running() {
        return Err("Failed to report running");
    }

    while !STOP.load(Ordering::Relaxed) {
        std::thread::sleep(std::time::Duration::from_millis(10));
    }
    Ok(())
}
