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
use crate::deadline::deadline_monitor::Deadline;
use crate::deadline::{DeadlineMonitor, DeadlineMonitorBuilder, DeadlineMonitorError};
use crate::ffi::{FFIBorrowed, FFICode, FFIHandle};
use crate::{IdentTag, TimeRange};
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

    pub(crate) fn get_deadline(&self, tag: IdentTag) -> Result<FFIHandle, FFICode> {
        match self.monitor.get_deadline(&tag) {
            Ok(deadline) => {
                // Now we allocate at runtime. As next step we will add a memory pool for deadlines into self and this way we will not need allocate anymore
                Ok(Box::into_raw(Box::new(deadline)).cast())
            },
            Err(DeadlineMonitorError::DeadlineInUse) => Err(FFICode::AlreadyExists),
            Err(DeadlineMonitorError::DeadlineNotFound) => Err(FFICode::NotFound),
        }
    }
}

#[no_mangle]
pub extern "C" fn deadline_monitor_builder_create(deadline_monitor_builder_handle_out: *mut FFIHandle) -> FFICode {
    if deadline_monitor_builder_handle_out.is_null() {
        return FFICode::NullParameter;
    }

    let deadline_monitor_builder = DeadlineMonitorBuilder::new();
    unsafe {
        *deadline_monitor_builder_handle_out = Box::into_raw(Box::new(deadline_monitor_builder)).cast();
    }

    FFICode::Success
}

#[no_mangle]
pub extern "C" fn deadline_monitor_builder_destroy(deadline_monitor_builder_handle: FFIHandle) -> FFICode {
    if deadline_monitor_builder_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `deadline_monitor_builder_create`.
    unsafe {
        let _ = Box::from_raw(deadline_monitor_builder_handle as *mut DeadlineMonitorBuilder);
    }

    FFICode::Success
}

#[no_mangle]
pub extern "C" fn deadline_monitor_builder_add_deadline(
    deadline_monitor_builder_handle: FFIHandle,
    deadline_tag: *const IdentTag,
    min_ms: u32,
    max_ms: u32,
) -> FFICode {
    if deadline_monitor_builder_handle.is_null() || deadline_tag.is_null() {
        return FFICode::NullParameter;
    }

    if min_ms > max_ms {
        return FFICode::InvalidArgument;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // `IdentTag` type must be compatible between C++ and Rust.
    let deadline_tag = unsafe { *deadline_tag };

    // SAFETY:
    // Validity of this pointer is ensured.
    // It is assumed that the pointer was created by a call to `deadline_monitor_builder_create`.
    // It is assumed that the pointer was not consumed by a call to `deadline_monitor_builder_destroy`.
    let mut deadline_monitor_builder =
        FFIBorrowed::new(unsafe { Box::from_raw(deadline_monitor_builder_handle as *mut DeadlineMonitorBuilder) });

    deadline_monitor_builder.add_deadline_internal(
        &deadline_tag,
        TimeRange::new(
            Duration::from_millis(min_ms as u64),
            Duration::from_millis(max_ms as u64),
        ),
    );

    FFICode::Success
}

#[no_mangle]
pub extern "C" fn deadline_monitor_get_deadline(
    deadline_monitor_handle: FFIHandle,
    deadline_tag: *const IdentTag,
    deadline_handle_out: *mut FFIHandle,
) -> FFICode {
    if deadline_monitor_handle.is_null() || deadline_tag.is_null() || deadline_handle_out.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // `IdentTag` type must be compatible between C++ and Rust.
    let deadline_tag = unsafe { *deadline_tag };

    // SAFETY:
    // Validity of this pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_get_deadline_monitor`.
    // It is assumed that the pointer was not consumed by a call to `deadline_monitor_destroy`.
    let deadline_monitor =
        FFIBorrowed::new(unsafe { Box::from_raw(deadline_monitor_handle as *mut DeadlineMonitorCpp) });

    match deadline_monitor.get_deadline(deadline_tag) {
        Ok(handle) => {
            unsafe {
                *deadline_handle_out = handle;
            }
            FFICode::Success
        },
        Err(e) => e,
    }
}

#[no_mangle]
pub extern "C" fn deadline_monitor_destroy(deadline_monitor_handle: FFIHandle) -> FFICode {
    if deadline_monitor_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_get_deadline_monitor`.
    unsafe {
        let _ = Box::from_raw(deadline_monitor_handle as *mut DeadlineMonitorCpp);
    }

    FFICode::Success
}

#[no_mangle]
pub extern "C" fn deadline_start(deadline_handle: FFIHandle) -> FFICode {
    if deadline_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of this pointer is ensured.
    // It is assumed that the pointer was created by a call to `deadline_monitor_get_deadline`.
    // It is assumed that the pointer was not consumed by a call to `deadline_destroy`.
    let mut deadline = FFIBorrowed::new(unsafe { Box::from_raw(deadline_handle as *mut Deadline) });

    // SAFETY: `Deadline` has move-only semantic, as multiple owners are not allowed.
    match unsafe { deadline.start_internal() } {
        Ok(()) => FFICode::Success,
        Err(_err) => FFICode::Failed,
    }
}

#[no_mangle]
pub extern "C" fn deadline_stop(deadline_handle: FFIHandle) -> FFICode {
    if deadline_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of this pointer is ensured.
    // It is assumed that the pointer was created by a call to `deadline_monitor_get_deadline`.
    // It is assumed that the pointer was not consumed by a call to `deadline_destroy`.
    let mut deadline = FFIBorrowed::new(unsafe { Box::from_raw(deadline_handle as *mut Deadline) });

    deadline.stop_internal();

    FFICode::Success
}

#[no_mangle]
pub extern "C" fn deadline_destroy(deadline_handle: FFIHandle) -> FFICode {
    if deadline_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of this pointer is ensured.
    // It is assumed that the pointer was created by a call to `deadline_monitor_get_deadline`.
    unsafe {
        let _ = Box::from_raw(deadline_handle as *mut Deadline);
    }

    FFICode::Success
}

#[score_testing_macros::test_mod_with_log]
#[cfg(test)]
mod tests {
    use crate::deadline::ffi::{
        deadline_destroy, deadline_monitor_builder_add_deadline, deadline_monitor_builder_create,
        deadline_monitor_builder_destroy, deadline_monitor_destroy, deadline_monitor_get_deadline, deadline_start,
        deadline_stop,
    };
    use crate::ffi::{
        health_monitor_builder_add_deadline_monitor, health_monitor_builder_build, health_monitor_builder_create,
        health_monitor_destroy, health_monitor_get_deadline_monitor, FFICode, FFIHandle,
    };
    use crate::IdentTag;
    use core::ptr::null_mut;

    #[test]
    fn deadline_monitor_builder_create_succeeds() {
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();

        let deadline_monitor_builder_create_result =
            deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        assert!(!deadline_monitor_builder_handle.is_null());
        assert_eq!(deadline_monitor_builder_create_result, FFICode::Success);

        // Clean-up.
        // NOTE: `deadline_monitor_builder_destroy` positive path is already tested here.
        let deadline_monitor_builder_destroy_result = deadline_monitor_builder_destroy(deadline_monitor_builder_handle);
        assert_eq!(deadline_monitor_builder_destroy_result, FFICode::Success);
    }

    #[test]
    fn deadline_monitor_builder_create_null_builder() {
        let deadline_monitor_builder_create_result = deadline_monitor_builder_create(null_mut());
        assert_eq!(deadline_monitor_builder_create_result, FFICode::NullParameter);
    }

    #[test]
    fn deadline_monitor_builder_destroy_null_builder() {
        let deadline_monitor_builder_destroy_result = deadline_monitor_builder_destroy(null_mut());
        assert_eq!(deadline_monitor_builder_destroy_result, FFICode::NullParameter);
    }

    #[test]
    fn deadline_monitor_builder_add_deadline_succeeds() {
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();

        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let deadline_tag = IdentTag::new("deadline_1");

        let deadline_monitor_builder_add_deadline_result = deadline_monitor_builder_add_deadline(
            deadline_monitor_builder_handle,
            &deadline_tag as *const IdentTag,
            100,
            200,
        );
        assert_eq!(deadline_monitor_builder_add_deadline_result, FFICode::Success);

        // Clean-up.
        deadline_monitor_builder_destroy(deadline_monitor_builder_handle);
    }

    #[test]
    fn deadline_monitor_builder_add_deadline_invalid_range() {
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();

        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let deadline_tag = IdentTag::new("deadline_1");

        let deadline_monitor_builder_add_deadline_result = deadline_monitor_builder_add_deadline(
            deadline_monitor_builder_handle,
            &deadline_tag as *const IdentTag,
            10000,
            100,
        );
        assert_eq!(deadline_monitor_builder_add_deadline_result, FFICode::InvalidArgument);

        // Clean-up.
        deadline_monitor_builder_destroy(deadline_monitor_builder_handle);
    }

    #[test]
    fn deadline_monitor_builder_add_deadline_null_builder() {
        let deadline_tag = IdentTag::new("deadline_1");

        let deadline_monitor_builder_add_deadline_result =
            deadline_monitor_builder_add_deadline(null_mut(), &deadline_tag as *const IdentTag, 100, 200);
        assert_eq!(deadline_monitor_builder_add_deadline_result, FFICode::NullParameter);
    }

    #[test]
    fn deadline_monitor_builder_add_deadline_null_deadline_tag() {
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();

        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);

        let deadline_monitor_builder_add_deadline_result =
            deadline_monitor_builder_add_deadline(deadline_monitor_builder_handle, null_mut(), 100, 200);
        assert_eq!(deadline_monitor_builder_add_deadline_result, FFICode::NullParameter);

        // Clean-up.
        deadline_monitor_builder_destroy(deadline_monitor_builder_handle);
    }

    #[test]
    fn deadline_monitor_get_deadline_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_handle: FFIHandle = null_mut();
        let mut deadline_handle: FFIHandle = null_mut();

        let deadline_monitor_tag = IdentTag::new("deadline_monitor");
        let deadline_tag = IdentTag::new("deadline_1");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_add_deadline(
            deadline_monitor_builder_handle,
            &deadline_tag as *const IdentTag,
            100,
            200,
        );
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const IdentTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );
        let _ = health_monitor_get_deadline_monitor(
            health_monitor_handle,
            &deadline_monitor_tag as *const IdentTag,
            &mut deadline_monitor_handle as *mut FFIHandle,
        );

        let deadline_monitor_get_deadline_result = deadline_monitor_get_deadline(
            deadline_monitor_handle,
            &deadline_tag as *const IdentTag,
            &mut deadline_handle as *mut FFIHandle,
        );
        assert!(!deadline_handle.is_null());
        assert_eq!(deadline_monitor_get_deadline_result, FFICode::Success);

        // Clean-up.
        deadline_destroy(deadline_handle);
        deadline_monitor_destroy(deadline_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn deadline_monitor_get_deadline_unknown_deadline() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_handle: FFIHandle = null_mut();
        let mut deadline_handle: FFIHandle = null_mut();

        let deadline_monitor_tag = IdentTag::new("deadline_monitor");
        let deadline_tag = IdentTag::new("deadline_1");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_add_deadline(
            deadline_monitor_builder_handle,
            &deadline_tag as *const IdentTag,
            100,
            200,
        );
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const IdentTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );
        let _ = health_monitor_get_deadline_monitor(
            health_monitor_handle,
            &deadline_monitor_tag as *const IdentTag,
            &mut deadline_monitor_handle as *mut FFIHandle,
        );

        let unknown_deadline_tag = IdentTag::new("deadline_2");
        let deadline_monitor_get_deadline_result = deadline_monitor_get_deadline(
            deadline_monitor_handle,
            &unknown_deadline_tag as *const IdentTag,
            &mut deadline_handle as *mut FFIHandle,
        );
        assert!(deadline_handle.is_null());
        assert_eq!(deadline_monitor_get_deadline_result, FFICode::NotFound);

        // Clean-up.
        deadline_monitor_destroy(deadline_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn deadline_monitor_get_deadline_null_monitor() {
        let mut deadline_handle: FFIHandle = null_mut();

        let deadline_tag = IdentTag::new("deadline_1");

        let deadline_monitor_get_deadline_result = deadline_monitor_get_deadline(
            null_mut(),
            &deadline_tag as *const IdentTag,
            &mut deadline_handle as *mut FFIHandle,
        );
        assert_eq!(deadline_monitor_get_deadline_result, FFICode::NullParameter);
    }

    #[test]
    fn deadline_monitor_get_deadline_null_deadline_tag() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_handle: FFIHandle = null_mut();
        let mut deadline_handle: FFIHandle = null_mut();

        let deadline_monitor_tag = IdentTag::new("deadline_monitor");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const IdentTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );
        let _ = health_monitor_get_deadline_monitor(
            health_monitor_handle,
            &deadline_monitor_tag as *const IdentTag,
            &mut deadline_monitor_handle as *mut FFIHandle,
        );

        let deadline_monitor_get_deadline_result = deadline_monitor_get_deadline(
            deadline_monitor_handle,
            null_mut(),
            &mut deadline_handle as *mut FFIHandle,
        );
        assert_eq!(deadline_monitor_get_deadline_result, FFICode::NullParameter);

        // Clean-up.
        deadline_monitor_destroy(deadline_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn deadline_monitor_get_deadline_null_deadline_handle() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_handle: FFIHandle = null_mut();

        let deadline_monitor_tag = IdentTag::new("deadline_monitor");
        let deadline_tag = IdentTag::new("deadline_1");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_add_deadline(
            deadline_monitor_builder_handle,
            &deadline_tag as *const IdentTag,
            100,
            200,
        );
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const IdentTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );
        let _ = health_monitor_get_deadline_monitor(
            health_monitor_handle,
            &deadline_monitor_tag as *const IdentTag,
            &mut deadline_monitor_handle as *mut FFIHandle,
        );

        let deadline_monitor_get_deadline_result =
            deadline_monitor_get_deadline(deadline_monitor_handle, &deadline_tag as *const IdentTag, null_mut());
        assert_eq!(deadline_monitor_get_deadline_result, FFICode::NullParameter);

        // Clean-up.
        deadline_monitor_destroy(deadline_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn deadline_monitor_destroy_null_monitor() {
        let deadline_monitor_destroy_result = deadline_monitor_destroy(null_mut());
        assert_eq!(deadline_monitor_destroy_result, FFICode::NullParameter);
    }

    #[test]
    fn deadline_start_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_handle: FFIHandle = null_mut();
        let mut deadline_handle: FFIHandle = null_mut();

        let deadline_monitor_tag = IdentTag::new("deadline_monitor");
        let deadline_tag = IdentTag::new("deadline_1");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_add_deadline(
            deadline_monitor_builder_handle,
            &deadline_tag as *const IdentTag,
            100,
            200,
        );
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const IdentTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );
        let _ = health_monitor_get_deadline_monitor(
            health_monitor_handle,
            &deadline_monitor_tag as *const IdentTag,
            &mut deadline_monitor_handle as *mut FFIHandle,
        );
        let _ = deadline_monitor_get_deadline(
            deadline_monitor_handle,
            &deadline_tag as *const IdentTag,
            &mut deadline_handle as *mut FFIHandle,
        );

        let deadline_start_result = deadline_start(deadline_handle);
        assert_eq!(deadline_start_result, FFICode::Success);

        // Clean-up.
        deadline_destroy(deadline_handle);
        deadline_monitor_destroy(deadline_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn deadline_start_already_started() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_handle: FFIHandle = null_mut();
        let mut deadline_handle: FFIHandle = null_mut();

        let deadline_monitor_tag = IdentTag::new("deadline_monitor");
        let deadline_tag = IdentTag::new("deadline_1");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_add_deadline(
            deadline_monitor_builder_handle,
            &deadline_tag as *const IdentTag,
            100,
            200,
        );
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const IdentTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );
        let _ = health_monitor_get_deadline_monitor(
            health_monitor_handle,
            &deadline_monitor_tag as *const IdentTag,
            &mut deadline_monitor_handle as *mut FFIHandle,
        );
        let _ = deadline_monitor_get_deadline(
            deadline_monitor_handle,
            &deadline_tag as *const IdentTag,
            &mut deadline_handle as *mut FFIHandle,
        );

        let _ = deadline_start(deadline_handle);
        let deadline_start_result = deadline_start(deadline_handle);
        assert_eq!(deadline_start_result, FFICode::Failed);

        // Clean-up.
        deadline_destroy(deadline_handle);
        deadline_monitor_destroy(deadline_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn deadline_start_null_deadline() {
        let deadline_start_result = deadline_start(null_mut());
        assert_eq!(deadline_start_result, FFICode::NullParameter);
    }

    #[test]
    fn deadline_stop_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_handle: FFIHandle = null_mut();
        let mut deadline_handle: FFIHandle = null_mut();

        let deadline_monitor_tag = IdentTag::new("deadline_monitor");
        let deadline_tag = IdentTag::new("deadline_1");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_add_deadline(
            deadline_monitor_builder_handle,
            &deadline_tag as *const IdentTag,
            100,
            200,
        );
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const IdentTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );
        let _ = health_monitor_get_deadline_monitor(
            health_monitor_handle,
            &deadline_monitor_tag as *const IdentTag,
            &mut deadline_monitor_handle as *mut FFIHandle,
        );
        let _ = deadline_monitor_get_deadline(
            deadline_monitor_handle,
            &deadline_tag as *const IdentTag,
            &mut deadline_handle as *mut FFIHandle,
        );
        let _ = deadline_start(deadline_handle);

        let deadline_stop_result = deadline_stop(deadline_handle);
        assert_eq!(deadline_stop_result, FFICode::Success);

        // Clean-up.
        deadline_destroy(deadline_handle);
        deadline_monitor_destroy(deadline_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn deadline_stop_null_deadline() {
        let deadline_stop_result = deadline_stop(null_mut());
        assert_eq!(deadline_stop_result, FFICode::NullParameter);
    }

    #[test]
    fn deadline_destroy_null_deadline() {
        let deadline_destroy_result = deadline_destroy(null_mut());
        assert_eq!(deadline_destroy_result, FFICode::NullParameter);
    }
}
