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
#![allow(unused_imports)]
#![allow(dead_code)]
use clap::Parser;
use health_monitoring_lib::*;
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

    /// The app is configured to measure deadline between 50ms and 150ms. You configure the delay inside this deadline measurement.
    #[arg(short, long)]
    delay: u32,
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

fn main_logic(args: &Args, stop: Arc<AtomicBool>) -> Result<(), Box<dyn std::error::Error>> {
    let mut builder = deadline::DeadlineMonitorBuilder::new();
    builder = builder.add_deadline(
        &IdentTag::from("deadline1"),
        TimeRange::new(
            std::time::Duration::from_millis(50),
            std::time::Duration::from_millis(150),
        ),
    );

    let mut hm = HealthMonitorBuilder::new()
        .add_deadline_monitor(&IdentTag::from("mon1"), builder)
        .build();

    let mon = hm
        .get_deadline_monitor(&IdentTag::from("mon1"))
        .expect("Failed to get monitor");

    hm.start();
    while !stop.load(Ordering::Relaxed) {
        let mut deadline = mon
            .get_deadline(&IdentTag::from("deadline1"))
            .expect("Failed to get deadline");

        let res = deadline.start();
        std::thread::sleep(std::time::Duration::from_millis(args.delay.into()));

        drop(res); // Ensure the deadline is ended when going out of scope
    }

    Ok(())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    use stdout_logger;
    stdout_logger::StdoutLoggerBuilder::new().set_as_default_logger();

    set_process_name();

    let args = Args::parse();
    let stop = Arc::new(AtomicBool::new(false));
    flag::register(signal_hook::consts::SIGTERM, Arc::clone(&stop))?;

    main_logic(&args, stop.clone())
}
