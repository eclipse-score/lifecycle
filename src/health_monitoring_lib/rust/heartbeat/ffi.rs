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

use crate::ffi::FFIHandle;
use crate::heartbeat::{HeartbeatMonitor, HeartbeatMonitorBuilder};
use crate::TimeRange;
use core::time::Duration;

#[no_mangle]
pub extern "C" fn heartbeat_monitor_builder_create(range_min_ms: u32, range_max_ms: u32) -> FFIHandle {
    let range_min = Duration::from_millis(range_min_ms as u64);
    let range_max = Duration::from_millis(range_max_ms as u64);
    let range = TimeRange::new(range_min, range_max);
    let builder = HeartbeatMonitorBuilder::new(range);
    let handle = Box::into_raw(Box::new(builder));
    handle as FFIHandle
}

#[no_mangle]
pub extern "C" fn heartbeat_monitor_builder_destroy(monitor_builder_handle: FFIHandle) {
    assert!(!monitor_builder_handle.is_null());

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that pointer was created by a call to `heartbeat_monitor_builder_create`.
    unsafe {
        let _ = Box::from_raw(monitor_builder_handle as *mut HeartbeatMonitorBuilder);
    }
}

#[no_mangle]
pub extern "C" fn heartbeat_monitor_destroy(monitor_handle: FFIHandle) {
    assert!(!monitor_handle.is_null());

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_get_heartbeat_monitor`.
    unsafe {
        let _ = Box::from_raw(monitor_handle as *mut HeartbeatMonitor);
    }
}

#[no_mangle]
pub extern "C" fn heartbeat_monitor_heartbeat(monitor_handle: FFIHandle) {
    assert!(!monitor_handle.is_null());

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_get_heartbeat_monitor`.
    let monitor = unsafe { Box::from_raw(monitor_handle as *mut HeartbeatMonitor) };

    monitor.heartbeat();
}
