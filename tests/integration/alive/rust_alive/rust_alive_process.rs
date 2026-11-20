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
/// Optional argument "--period_ms=<ms>": time between two alive notifications
/// (default 50).
fn main() -> Result<(), Box<dyn std::error::Error>> {
    const USAGE: &str = "Usage: rust_alive_process [--reports=<n>] [--period_ms=<ms>]";
    let mut remaining_reports: Option<u64> = None;
    let mut period_ms: u64 = 50;
    for arg in std::env::args().skip(1) {
        if let Some(n) = arg.strip_prefix("--reports=") {
            remaining_reports = Some(n.parse().map_err(|_| USAGE)?);
        } else if let Some(ms) = arg.strip_prefix("--period_ms=") {
            period_ms = ms.parse().map_err(|_| USAGE)?;
        } else {
            return Err(USAGE.into());
        }
    }

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
        std::thread::sleep(std::time::Duration::from_millis(period_ms));
    }
    Ok(())
}
