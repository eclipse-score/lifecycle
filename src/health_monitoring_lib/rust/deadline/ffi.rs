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
use crate::tag::DeadlineTag;
use crate::*;
use core::time::Duration;
use std::os::raw::c_int;

pub(crate) struct DeadlineMonitorCpp {
    monitor: DeadlineMonitor,
    // TODO: Here we will keep allocation storage for Deadlines once we implement memory pool
    // For now, Deadlines are kept allocated on heap individually
}

impl DeadlineMonitorCpp {
    pub(crate) fn new(monitor: DeadlineMonitor) -> Self {
        Self { monitor }
    }

    pub(crate) fn get_deadline(&self, deadline_tag: DeadlineTag) -> Result<FFIHandle, c_int> {
        match self.monitor.get_deadline(deadline_tag) {
            Ok(deadline) => {
                // Now we allocate at runtime. As next step we will add a memory pool for deadlines into self and this way we will not need allocate anymore
                let handle = Box::into_raw(Box::new(deadline));
                Ok(handle as FFIHandle)
            },
            Err(DeadlineMonitorError::DeadlineInUse) => Err(HM_ALREADY_EXISTS),
            Err(DeadlineMonitorError::DeadlineNotFound) => Err(HM_NOT_FOUND),
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
pub extern "C" fn deadline_monitor_builder_destroy(monitor_builder_handle: FFIHandle) {
    assert!(!monitor_builder_handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_builder_create`
    // and this must be assured on other side of FFI.
    unsafe {
        let _ = Box::from_raw(monitor_builder_handle as *mut DeadlineMonitorBuilder);
    }
}

#[no_mangle]
pub extern "C" fn deadline_monitor_builder_add_deadline(
    monitor_builder_handle: FFIHandle,
    deadline_tag: *const DeadlineTag,
    min: u32,
    max: u32,
) {
    assert!(!monitor_builder_handle.is_null());
    assert!(!deadline_tag.is_null());

    // Safety: We ensure that the pointer is valid. `deadline_tag` ptr must be FFI data compatible with DeadlineTag in Rust
    let deadline_tag = unsafe { *deadline_tag }; // Copy the DeadlineTag as this shall be trivially copyable

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_builder_create`
    // and this must be assured on other side of FFI.
    let mut monitor = FFIBorrowed::new(unsafe { Box::from_raw(monitor_builder_handle as *mut DeadlineMonitorBuilder) });

    monitor.add_deadline_internal(
        deadline_tag,
        TimeRange::new(Duration::from_millis(min as u64), Duration::from_millis(max as u64)),
    );
}

#[no_mangle]
pub extern "C" fn deadline_monitor_cpp_destroy(monitor_handle: FFIHandle) {
    assert!(!monitor_handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_builder_create`
    // and this must be assured on other side of FFI.
    unsafe {
        let _ = Box::from_raw(monitor_handle as *mut DeadlineMonitorCpp);
    }
}

#[no_mangle]
pub extern "C" fn deadline_monitor_cpp_get_deadline(
    monitor_handle: FFIHandle,
    deadline_tag: *const DeadlineTag,
    out: *mut FFIHandle,
) -> c_int {
    assert!(!monitor_handle.is_null());
    assert!(!deadline_tag.is_null());
    assert!(!out.is_null());

    // Safety: We ensure that the pointer is valid. `deadline_tag` ptr must be FFI data compatible with DeadlineTag in Rust
    let deadline_tag = unsafe { *deadline_tag }; // Copy the DeadlineTag as this shall be trivially copyable

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_builder_create`
    // and this must be assured on other side of FFI.
    let monitor = FFIBorrowed::new(unsafe { Box::from_raw(monitor_handle as *mut DeadlineMonitorCpp) });
    let deadline_handle = monitor.get_deadline(deadline_tag);

    deadline_handle.map_or_else(
        |err_code| err_code,
        |handle| {
            unsafe {
                *out = handle;
            }
            HM_OK
        },
    )
}

#[no_mangle]
pub extern "C" fn deadline_start(deadline_handle: FFIHandle) -> c_int {
    assert!(!deadline_handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_cpp_get_deadline`
    // and this must be assured on other side of FFI.
    let mut deadline = FFIBorrowed::new(unsafe { Box::from_raw(deadline_handle as *mut Deadline) });

    // Safety: We ensure at CPP side that a Deadline  has move only semantic to not end up in multiple owners of same deadline.
    // We also check during start call that previous start/stop sequence was done correctly.
    match unsafe { deadline.start_internal() } {
        Ok(()) => HM_OK,
        Err(_err) => HM_FAILED,
    }
}

#[no_mangle]
pub extern "C" fn deadline_stop(deadline_handle: FFIHandle) {
    assert!(!deadline_handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_cpp_get_deadline`
    // and this must be assured on other side of FFI.
    let mut deadline = FFIBorrowed::new(unsafe { Box::from_raw(deadline_handle as *mut Deadline) });
    deadline.stop_internal();
}

#[no_mangle]
pub extern "C" fn deadline_destroy(deadline_handle: FFIHandle) {
    assert!(!deadline_handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_cpp_get_deadline`
    // and this must be assured on other side of FFI.
    unsafe {
        let _ = Box::from_raw(deadline_handle as *mut Deadline);
    }
}
