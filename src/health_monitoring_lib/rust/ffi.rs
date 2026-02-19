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
use crate::deadline::ffi::DeadlineMonitorCpp;
use crate::*;
use core::mem::ManuallyDrop;
use core::ops::{Deref, DerefMut};
use core::time::Duration;

pub(crate) type FFIHandle = *mut core::ffi::c_void;

pub(crate) const HM_OK: i32 = 0;
pub(crate) const HM_NOT_FOUND: i32 = HM_OK + 1;
pub(crate) const HM_ALREADY_EXISTS: i32 = HM_OK + 2;
pub(crate) const _HM_INVALID_ARGS: i32 = HM_OK + 3;
pub(crate) const _HM_WRONG_STATE: i32 = HM_OK + 4;
pub(crate) const HM_FAILED: i32 = HM_OK + 5;

/// A wrapper to represent borrowed data over FFI boundary without taking ownership.
pub(crate) struct FFIBorrowed<T> {
    data: ManuallyDrop<T>,
}

impl<T> FFIBorrowed<T> {
    pub(crate) fn new(data: T) -> Self {
        Self {
            data: ManuallyDrop::new(data),
        }
    }
}

impl<T: Deref> Deref for FFIBorrowed<T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        &self.data
    }
}

impl<T: DerefMut> DerefMut for FFIBorrowed<T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.data
    }
}

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
extern "C" fn health_monitor_builder_add_deadline_monitor(
    handle: FFIHandle,
    monitor_tag: *const MonitorTag,
    monitor: FFIHandle,
) {
    assert!(!handle.is_null());
    assert!(!monitor_tag.is_null());
    assert!(!monitor.is_null());

    // Safety: We ensure that the pointer is valid. `monitor_tag` ptr must be FFI data compatible with MonitorTag in Rust
    let tag = unsafe { *monitor_tag }; // Copy the MonitorTag as this shall be trivially copyable

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_builder_create`
    let monitor = unsafe { Box::from_raw(monitor as *mut DeadlineMonitorBuilder) };

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_create`
    // and this must be assured on other side of FFI.
    let mut health_monitor_builder = FFIBorrowed::new(unsafe { Box::from_raw(handle as *mut HealthMonitorBuilder) });

    health_monitor_builder.add_deadline_monitor_internal(&tag, *monitor);
}

#[no_mangle]
extern "C" fn health_monitor_builder_add_heartbeat_monitor(
    hmon_builder_handle: FFIHandle,
    monitor_tag: *const MonitorTag,
    monitor_builder_handle: FFIHandle,
) {
    assert!(!hmon_builder_handle.is_null());
    assert!(!monitor_tag.is_null());
    assert!(!monitor_builder_handle.is_null());

    // SAFETY:
    // Validity of the pointer is ensured.
    // `MonitorTag` type must be compatible between C++ and Rust.
    let monitor_tag = unsafe { *monitor_tag };

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that pointer was created with a call to `heartbeat_monitor_builder_create`.
    let monitor_builder = unsafe { Box::from_raw(monitor_builder_handle as *mut HeartbeatMonitorBuilder) };

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
extern "C" fn health_monitor_get_deadline_monitor(handle: FFIHandle, monitor_tag: *const MonitorTag) -> FFIHandle {
    assert!(!handle.is_null());
    assert!(!monitor_tag.is_null());

    // Safety: We ensure that the pointer is valid. `monitor_tag` ptr must be FFI data compatible with MonitorTag in Rust
    let monitor_tag = unsafe { *monitor_tag }; // Copy the MonitorTag as this shall be trivially copyable

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_create`
    // and this must be assured on other side of FFI.
    let mut health_monitor = FFIBorrowed::new(unsafe { Box::from_raw(handle as *mut HealthMonitor) });

    if let Some(deadline_monitor) = health_monitor.get_deadline_monitor(&monitor_tag) {
        let deadline_monitor_handle = Box::into_raw(Box::new(DeadlineMonitorCpp::new(deadline_monitor)));

        deadline_monitor_handle as FFIHandle
    } else {
        core::ptr::null_mut()
    }
}

#[no_mangle]
extern "C" fn health_monitor_get_heartbeat_monitor(
    hmon_handle: FFIHandle,
    monitor_tag: *const MonitorTag,
) -> FFIHandle {
    assert!(!hmon_handle.is_null());
    assert!(!monitor_tag.is_null());

    // SAFETY:
    // Validity of the pointer is ensured.
    // `MonitorTag` type must be compatible between C++ and Rust.
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

#[cfg(test)]
mod tests {
    use crate::deadline::ffi::{deadline_monitor_builder_create, deadline_monitor_cpp_destroy};
    use crate::ffi::{
        health_monitor_builder_add_deadline_monitor, health_monitor_builder_build, health_monitor_builder_create,
        health_monitor_builder_destroy, health_monitor_destroy, health_monitor_get_deadline_monitor,
        health_monitor_start,
    };
    use crate::IdentTag;

    #[test]
    fn health_monitor_builder_create_ok() {
        let health_monitor_builder_handle = health_monitor_builder_create();
        assert!(!health_monitor_builder_handle.is_null());

        // Clean-up.
        // NOTE: `health_monitor_builder_destroy` positive path is already tested here.
        health_monitor_builder_destroy(health_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_deadline_monitor_ok() {
        let health_monitor_builder_handle = health_monitor_builder_create();
        let deadline_monitor_tag = IdentTag::new("deadline_monitor");
        let deadline_monitor_builder_handle = deadline_monitor_builder_create();
        health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const IdentTag,
            deadline_monitor_builder_handle,
        );

        // Clean-up.
        health_monitor_builder_destroy(health_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_build_ok() {
        let health_monitor_builder_handle = health_monitor_builder_create();
        let health_monitor_handle = health_monitor_builder_build(health_monitor_builder_handle, 200, 100);
        assert!(!health_monitor_handle.is_null());

        // Clean-up.
        // NOTE: `health_monitor_destroy` positive path is already tested here.
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_deadline_monitor_ok() {
        let health_monitor_builder_handle = health_monitor_builder_create();
        let deadline_monitor_tag = IdentTag::new("deadline_monitor");
        let deadline_monitor_builder_handle = deadline_monitor_builder_create();
        health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const IdentTag,
            deadline_monitor_builder_handle,
        );
        let health_monitor_handle = health_monitor_builder_build(health_monitor_builder_handle, 200, 100);

        let deadline_monitor_handle =
            health_monitor_get_deadline_monitor(health_monitor_handle, &deadline_monitor_tag as *const IdentTag);
        assert!(!deadline_monitor_handle.is_null());

        // Clean-up.
        deadline_monitor_cpp_destroy(deadline_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_start_ok() {
        let health_monitor_builder_handle = health_monitor_builder_create();
        let deadline_monitor_tag = IdentTag::new("deadline_monitor");
        let deadline_monitor_builder_handle = deadline_monitor_builder_create();
        health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const IdentTag,
            deadline_monitor_builder_handle,
        );
        let health_monitor_handle = health_monitor_builder_build(health_monitor_builder_handle, 200, 100);

        let deadline_monitor_handle =
            health_monitor_get_deadline_monitor(health_monitor_handle, &deadline_monitor_tag as *const IdentTag);
        assert!(!deadline_monitor_handle.is_null());

        health_monitor_start(health_monitor_handle);

        // Clean-up.
        deadline_monitor_cpp_destroy(deadline_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }
}
