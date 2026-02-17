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
use crate::deadline::deadline_monitor::Deadline;
use crate::deadline::*;
use crate::*;
use core::time::Duration;

pub(crate) struct DeadlineMonitorCpp {
    monitor: DeadlineMonitor,
    // TODO: Here we will keep allocation storage for Deadlines once we implement memory pool
    // For now, Deadlines are kept allocated on heap individually
}

impl DeadlineMonitorCpp {
    pub(crate) fn new(monitor: DeadlineMonitor) -> Self {
        Self { monitor }
    }

    pub(crate) fn get_deadline(&self, tag: IdentTag) -> Result<FFIHandle, FFIError> {
        match self.monitor.get_deadline(&tag) {
            Ok(deadline) => {
                // Now we allocate at runtime. As next step we will add a memory pool for deadlines into self and this way we will not need allocate anymore
                let handle = Box::into_raw(Box::new(deadline));
                Ok(handle as FFIHandle)
            },
            Err(DeadlineMonitorError::DeadlineInUse) => Err(FFIError::AlreadyExists),
            Err(DeadlineMonitorError::DeadlineNotFound) => Err(FFIError::NotFound),
        }
    }
}

#[no_mangle]
pub extern "C" fn deadline_monitor_builder_create() -> FFIHandle {
    let builder = DeadlineMonitorBuilder::new();
    let handle = Box::into_raw(Box::new(builder));
    handle as FFIHandle
}

#[no_mangle]
pub extern "C" fn deadline_monitor_builder_destroy(handle: FFIHandle) {
    assert!(!handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_builder_create`
    // and this must be assured on other side of FFI.
    unsafe {
        let _ = Box::from_raw(handle as *mut DeadlineMonitorBuilder);
    }
}

#[no_mangle]
pub extern "C" fn deadline_monitor_builder_add_deadline(handle: FFIHandle, tag: *const IdentTag, min: u32, max: u32) {
    assert!(!handle.is_null());
    assert!(!tag.is_null());

    // Safety: We ensure that the pointer is valid. `tag` ptr must be FFI data compatible with IdentTag in Rust
    let tag: IdentTag = unsafe { *tag }; // Copy the IdentTag as this shall be trivially copyable

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_builder_create`
    // and this must be assured on other side of FFI.
    let mut monitor = FFIBorrowed::new(unsafe { Box::from_raw(handle as *mut DeadlineMonitorBuilder) });

    monitor.add_deadline_internal(
        &tag,
        TimeRange::new(Duration::from_millis(min as u64), Duration::from_millis(max as u64)),
    );
}

#[no_mangle]
pub extern "C" fn deadline_monitor_cpp_destroy(handle: FFIHandle) {
    assert!(!handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_builder_create`
    // and this must be assured on other side of FFI.
    unsafe {
        let _ = Box::from_raw(handle as *mut DeadlineMonitorCpp);
    }
}

#[no_mangle]
pub extern "C" fn deadline_monitor_cpp_get_deadline(
    handle: FFIHandle,
    tag: *const IdentTag,
    out: *mut FFIHandle,
) -> FFIError {
    assert!(!handle.is_null());
    assert!(!tag.is_null());
    assert!(!out.is_null());

    // Safety: We ensure that the pointer is valid. `tag` ptr must be FFI data compatible with IdentTag in Rust
    let tag: IdentTag = unsafe { *tag }; // Copy the IdentTag as this shall be trivially copyable

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_builder_create`
    // and this must be assured on other side of FFI.
    let monitor = FFIBorrowed::new(unsafe { Box::from_raw(handle as *mut DeadlineMonitorCpp) });
    let deadline_handle = monitor.get_deadline(tag);

    deadline_handle.map_or_else(
        |err_code| err_code,
        |handle| {
            unsafe {
                *out = handle;
            }
            FFIError::Success
        },
    )
}

#[no_mangle]
pub extern "C" fn deadline_start(handle: FFIHandle) -> FFIError {
    assert!(!handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_cpp_get_deadline`
    // and this must be assured on other side of FFI.
    let mut deadline = FFIBorrowed::new(unsafe { Box::from_raw(handle as *mut Deadline) });

    // Safety: We ensure at CPP side that a Deadline  has move only semantic to not end up in multiple owners of same deadline.
    // We also check during start call that previous start/stop sequence was done correctly.
    match unsafe { deadline.start_internal() } {
        Ok(()) => FFIError::Success,
        Err(_err) => FFIError::Failed,
    }
}

#[no_mangle]
pub extern "C" fn deadline_stop(handle: FFIHandle) {
    assert!(!handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_cpp_get_deadline`
    // and this must be assured on other side of FFI.
    let mut deadline = FFIBorrowed::new(unsafe { Box::from_raw(handle as *mut Deadline) });
    deadline.stop_internal();
}

#[no_mangle]
pub extern "C" fn deadline_destroy(handle: FFIHandle) {
    assert!(!handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_cpp_get_deadline`
    // and this must be assured on other side of FFI.
    unsafe {
        let _ = Box::from_raw(handle as *mut Deadline);
    }
}
