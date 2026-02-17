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
use crate::common::ffi::*;
use crate::deadline::ffi::DeadlineMonitorCpp;
use crate::*;
use core::time::Duration;

#[no_mangle]
extern "C" fn health_monitor_builder_create() -> FFIHandle {
    let builder = HealthMonitorBuilder::new();
    let handle = Box::into_raw(Box::new(builder));
    handle as FFIHandle
}

#[no_mangle]
extern "C" fn health_monitor_builder_destroy(handle: FFIHandle) {
    assert!(!handle.is_null());
    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_create`
    // and this must be assured on other side of FFI.
    unsafe {
        let _ = Box::from_raw(handle as *mut HealthMonitorBuilder);
    }
}

#[no_mangle]
extern "C" fn health_monitor_builder_add_deadline_monitor(handle: FFIHandle, tag: *const IdentTag, monitor: FFIHandle) {
    assert!(!handle.is_null());
    assert!(!tag.is_null());
    assert!(!monitor.is_null());

    // Safety: We ensure that the pointer is valid. `tag` ptr must be FFI data compatible with IdentTag in Rust
    let tag: IdentTag = unsafe { *tag }; // Copy the IdentTag as this shall be trivially copyable

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_builder_create`
    let monitor = unsafe { Box::from_raw(monitor as *mut deadline::DeadlineMonitorBuilder) };

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_create`
    // and this must be assured on other side of FFI.
    let mut health_monitor_builder = FFIBorrowed::new(unsafe { Box::from_raw(handle as *mut HealthMonitorBuilder) });

    health_monitor_builder.add_deadline_monitor_internal(&tag, *monitor);
}

#[no_mangle]
extern "C" fn health_monitor_builder_add_heartbeat_monitor(
    hmon_builder_handle: FFIHandle,
    monitor_tag: *const IdentTag,
    monitor_builder_handle: FFIHandle,
) {
    assert!(!hmon_builder_handle.is_null());
    assert!(!monitor_tag.is_null());
    assert!(!monitor_builder_handle.is_null());

    // SAFETY:
    // Validity of the pointer is ensured.
    // `IdentTag` type must be compatible between C++ and Rust.
    let monitor_tag = unsafe { *monitor_tag };

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that pointer was created with a call to `heartbeat_monitor_builder_create`.
    let monitor_builder = unsafe { Box::from_raw(monitor_builder_handle as *mut heartbeat::HeartbeatMonitorBuilder) };

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that pointer was created with a call to `health_monitor_builder_create`.
    let mut health_monitor_builder =
        FFIBorrowed::new(unsafe { Box::from_raw(hmon_builder_handle as *mut HealthMonitorBuilder) });

    health_monitor_builder.add_heartbeat_monitor_internal(&monitor_tag, *monitor_builder);
}

#[no_mangle]
extern "C" fn health_monitor_builder_build(
    handle: FFIHandle,
    supervisor_cycle_ms: u32,
    internal_cycle_ms: u32,
) -> FFIHandle {
    assert!(!handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_create`
    // and this must be assured on other side of FFI.
    let mut health_monitor_builder: Box<HealthMonitorBuilder> =
        unsafe { Box::from_raw(handle as *mut HealthMonitorBuilder) };

    health_monitor_builder.with_internal_processing_cycle_internal(Duration::from_millis(internal_cycle_ms as u64));
    health_monitor_builder.with_supervisor_api_cycle_internal(Duration::from_millis(supervisor_cycle_ms as u64));

    let health_monitor = health_monitor_builder.build();
    let health_monitor_handle = Box::into_raw(Box::new(health_monitor));
    health_monitor_handle as FFIHandle
}

#[no_mangle]
extern "C" fn health_monitor_get_deadline_monitor(handle: FFIHandle, tag: *const IdentTag) -> FFIHandle {
    assert!(!handle.is_null());
    assert!(!tag.is_null());

    // Safety: We ensure that the pointer is valid. `tag` ptr must be FFI data compatible with IdentTag in Rust
    let tag: IdentTag = unsafe { *tag }; // Copy the IdentTag as this shall be trivially copyable

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_create`
    // and this must be assured on other side of FFI.
    let mut health_monitor = FFIBorrowed::new(unsafe { Box::from_raw(handle as *mut HealthMonitor) });

    if let Some(deadline_monitor) = health_monitor.get_deadline_monitor(&tag) {
        let deadline_monitor_handle = Box::into_raw(Box::new(DeadlineMonitorCpp::new(deadline_monitor)));

        deadline_monitor_handle as FFIHandle
    } else {
        core::ptr::null_mut()
    }
}

#[no_mangle]
extern "C" fn health_monitor_get_heartbeat_monitor(hmon_handle: FFIHandle, monitor_tag: *const IdentTag) -> FFIHandle {
    assert!(!hmon_handle.is_null());
    assert!(!monitor_tag.is_null());

    // SAFETY:
    // Validity of the pointer is ensured.
    // `IdentTag` type must be compatible between C++ and Rust.
    let monitor_tag = unsafe { *monitor_tag };

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that pointer was created with a call to `health_monitor_builder_build`.
    let mut health_monitor = FFIBorrowed::new(unsafe { Box::from_raw(hmon_handle as *mut HealthMonitor) });

    if let Some(heartbeat_monitor) = health_monitor.get_heartbeat_monitor(&monitor_tag) {
        Box::into_raw(Box::new(heartbeat_monitor)) as FFIHandle
    } else {
        core::ptr::null_mut()
    }
}

#[no_mangle]
extern "C" fn health_monitor_start(handle: FFIHandle) {
    assert!(!handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_build`
    // and this must be assured on other side of FFI.
    let mut monitor = FFIBorrowed::new(unsafe { Box::from_raw(handle as *mut HealthMonitor) });
    monitor.start();
}

#[no_mangle]
extern "C" fn health_monitor_destroy(handle: FFIHandle) {
    assert!(!handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_build`
    // and this must be assured on other side of FFI.
    unsafe {
        let _ = Box::from_raw(handle as *mut HealthMonitor);
    }
}
