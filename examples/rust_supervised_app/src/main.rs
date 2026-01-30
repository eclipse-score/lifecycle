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
use clap::Parser;
use libc::{c_long, nanosleep, time_t, timespec};
use signal_hook::flag;
use std::env;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    #[arg(short, long)]
    specifier: String,

    #[arg(short, long, default_value_t = 50)]
    delay: u32,
}

#[derive(Debug, Copy, Clone)]
enum Checks {
    One = 1,
    Two = 2,
    Three = 3,
}

impl From<Checks> for u32 {
    fn from(val: Checks) -> Self {
        val as u32
    }
}

fn interruptible_sleep(delay: timespec) {
    unsafe {
        nanosleep(&delay as *const timespec, std::ptr::null_mut());
    }
}

fn set_process_name() {
    if let Ok(val) = env::var("PROCESSIDENTIFIER") {
        let str = std::ffi::CString::new(val).expect("CString::new failed");
        #[cfg(target_os = "linux")]
        {
            unsafe {
                libc::prctl(libc::PR_SET_NAME, str.as_ptr());
            }
        }

        #[cfg(target_os = "nto")]
        {
            unsafe {
                libc::pthread_setname_np(libc::pthread_self(), str.as_ptr());
            }
        }
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    set_process_name();

    let args = Args::parse();
    let stop = Arc::new(AtomicBool::new(false));
    let stop_reporting_checkpoints = Arc::new(AtomicBool::new(false));
    flag::register(signal_hook::consts::SIGTERM, Arc::clone(&stop))?;
    flag::register(signal_hook::consts::SIGUSR1, Arc::clone(&stop_reporting_checkpoints))?;

        drop(res); // Ensure the deadline is ended when going out of scope, this keeps drop of Result here and not before timeout
        println!("Rust app FAILED to report execution state!");
    }

    let monitor = monitor_rs::Monitor::<Checks>::new(&args.specifier)?;

    let secs = (args.delay / 1000) as time_t;
    let nanos = ((args.delay % 1000) * 1_000_000) as c_long;

    let sleep_time = timespec {
        tv_sec: secs,
        tv_nsec: nanos,
    };

    while !stop.load(Ordering::Relaxed) {
        if !stop_reporting_checkpoints.load(Ordering::Relaxed) {
            monitor.report_checkpoint(Checks::One);
            interruptible_sleep(sleep_time);
            if stop.load(Ordering::Relaxed) {
                break;
            }
            monitor.report_checkpoint(Checks::Two);
            monitor.report_checkpoint(Checks::Three);
        } else {
            interruptible_sleep(sleep_time);
        }
    }

    Ok(())
}
