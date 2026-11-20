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

use std::sync::atomic::{AtomicBool, Ordering};

const SIGTERM: i32 = 15;

static STOP: AtomicBool = AtomicBool::new(false);

unsafe extern "C" {
    fn signal(signum: i32, handler: extern "C" fn(i32)) -> usize;
}

extern "C" fn sigterm_handler(_: i32) {
    STOP.store(true, Ordering::Relaxed);
}

/// Reports running, then reports alive every 50ms until SIGTERM is received.
/// Optional argument "--reports=<n>": only the first `n` alive notifications are
/// reported.
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut remaining_reports: Option<u64> = match std::env::args().nth(1) {
        None => None,
        Some(arg) => Some(
            arg.strip_prefix("--reports=")
                .and_then(|n| n.parse().ok())
                .ok_or("Usage: rust_alive_process [--reports=<n>]")?,
        ),
    };

    unsafe {
        signal(SIGTERM, sigterm_handler);
    }

    let alive = alive_rs::Alive::new("rust_alive_process")?;
    if !lifecycle_client_rs::report_running() {
        return Err("Failed to report running".into());
    }

    while !STOP.load(Ordering::Relaxed) {
        match remaining_reports.as_mut() {
            Some(0) => {},
            Some(n) => {
                alive.report_alive();
                *n -= 1;
            },
            None => alive.report_alive(),
        }
        std::thread::sleep(std::time::Duration::from_millis(50));
    }
    Ok(())
}
