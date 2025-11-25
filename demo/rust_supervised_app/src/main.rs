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

use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
use signal_hook::flag;
use std::convert::Into;
use clap::Parser;
use libc::{c_long, time_t, nanosleep, timespec};
use std::env;
use libc;

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
    KOne,
    KTwo,
    KThree
}

impl Into<u32> for Checks {
    fn into(self) -> u32 {
        match self {
            Checks::KOne => 1,
            Checks::KTwo => 2,
            Checks::KThree => 3,
        }
    }
}

fn interruptible_sleep(delay: timespec) {
    unsafe {
        nanosleep(&delay as *const timespec, std::ptr::null_mut());
    }
}

fn set_process_name() {
    match env::var("PROCESSIDENTIFIER") {
        Ok(val) => {
            let str = std::ffi::CString::new(val).expect("CString::new failed");
            unsafe {
                libc::prctl(libc::PR_SET_NAME, str.as_ptr());
            }
        }
        _ => {}
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    set_process_name();

    let args = Args::parse();
    let stop = Arc::new(AtomicBool::new(false));
    let stop_reporting_checkpoints = Arc::new(AtomicBool::new(false));
    flag::register(signal_hook::consts::SIGTERM, Arc::clone(&stop))?;
    flag::register(signal_hook::consts::SIGUSR1, Arc::clone(&stop_reporting_checkpoints))?;

    if !lifecycle_client_rs::report_execution_state_running() {
        println!("Rust app FAILED to report execution state!");
    }

    let monitor = monitor_rs::Monitor::<Checks>::new(&args.specifier)?;

    let secs = (args.delay / 1000) as time_t;
    let nanos = ((args.delay % 1000) * 1_000_000) as c_long;

    let sleep_time = timespec { tv_sec: secs, tv_nsec: nanos };

    while !stop.load(Ordering::Relaxed) {
        if !stop_reporting_checkpoints.load(Ordering::Relaxed) {
            monitor.report_checkpoint(Checks::KOne);
            interruptible_sleep(sleep_time);
            if stop.load(Ordering::Relaxed) {
                break;
            }
            monitor.report_checkpoint(Checks::KTwo);
            monitor.report_checkpoint(Checks::KThree);
        } else {
            interruptible_sleep(sleep_time);
        }
    }

    Ok(())
}
