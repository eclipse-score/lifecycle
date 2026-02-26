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
use crate::common::TimeRange;
use crate::ffi::{FFIBorrowed, FFICode, FFIHandle};
use crate::heartbeat::{HeartbeatMonitor, HeartbeatMonitorBuilder};
use core::time::Duration;

#[unsafe(no_mangle)]
pub extern "C" fn heartbeat_monitor_builder_create(
    range_min_ms: u32,
    range_max_ms: u32,
    heartbeat_monitor_builder_handle_out: *mut FFIHandle,
) -> FFICode {
    if heartbeat_monitor_builder_handle_out.is_null() {
        return FFICode::NullParameter;
    }

    let range_min = Duration::from_millis(range_min_ms as u64);
    let range_max = Duration::from_millis(range_max_ms as u64);
    let range = match TimeRange::new_internal(range_min, range_max) {
        Some(range) => range,
        None => return FFICode::InvalidArgument,
    };

    let heartbeat_monitor_builder = HeartbeatMonitorBuilder::new(range);
    unsafe {
        *heartbeat_monitor_builder_handle_out = Box::into_raw(Box::new(heartbeat_monitor_builder)).cast();
    }

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn heartbeat_monitor_builder_destroy(heartbeat_monitor_builder_handle: FFIHandle) -> FFICode {
    if heartbeat_monitor_builder_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `heartbeat_monitor_builder_create`.
    unsafe {
        let _ = Box::from_raw(heartbeat_monitor_builder_handle as *mut HeartbeatMonitorBuilder);
    }

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn heartbeat_monitor_destroy(heartbeat_monitor_handle: FFIHandle) -> FFICode {
    if heartbeat_monitor_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_get_heartbeat_monitor`.
    unsafe {
        let _ = Box::from_raw(heartbeat_monitor_handle as *mut HeartbeatMonitor);
    }

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn heartbeat_monitor_heartbeat(heartbeat_monitor_handle: FFIHandle) -> FFICode {
    if heartbeat_monitor_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of this pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_get_heartbeat_monitor`.
    // It is assumed that the pointer was not consumed by a call to `heartbeat_monitor_destroy`.
    let monitor = FFIBorrowed::new(unsafe { Box::from_raw(heartbeat_monitor_handle as *mut HeartbeatMonitor) });

    monitor.heartbeat();

    FFICode::Success
}

#[score_testing_macros::test_mod_with_log]
#[cfg(all(test, not(loom)))]
mod tests {
    use crate::ffi::{
        health_monitor_builder_add_heartbeat_monitor, health_monitor_builder_build, health_monitor_builder_create,
        health_monitor_destroy, health_monitor_get_heartbeat_monitor, FFICode, FFIHandle,
    };
    use crate::heartbeat::ffi::{
        heartbeat_monitor_builder_create, heartbeat_monitor_builder_destroy, heartbeat_monitor_destroy,
        heartbeat_monitor_heartbeat,
    };
    use crate::tag::MonitorTag;
    use core::ptr::null_mut;

    #[test]
    fn heartbeat_monitor_builder_create_succeeds() {
        let mut heartbeat_monitor_builder_handle: FFIHandle = null_mut();

        let heartbeat_monitor_builder_create_result =
            heartbeat_monitor_builder_create(100, 200, &mut heartbeat_monitor_builder_handle as *mut FFIHandle);
        assert!(!heartbeat_monitor_builder_handle.is_null());
        assert_eq!(heartbeat_monitor_builder_create_result, FFICode::Success);

        // Clean-up.
        // NOTE: `heartbeat_monitor_builder_destroy` positive path is already tested here.
        let heartbeat_monitor_builder_destroy_result =
            heartbeat_monitor_builder_destroy(heartbeat_monitor_builder_handle);
        assert_eq!(heartbeat_monitor_builder_destroy_result, FFICode::Success);
    }

    #[test]
    fn heartbeat_monitor_builder_create_invalid_range() {
        let mut heartbeat_monitor_builder_handle: FFIHandle = null_mut();

        let heartbeat_monitor_builder_create_result =
            heartbeat_monitor_builder_create(10000, 200, &mut heartbeat_monitor_builder_handle as *mut FFIHandle);
        assert_eq!(heartbeat_monitor_builder_create_result, FFICode::InvalidArgument);
    }

    #[test]
    fn heartbeat_monitor_builder_create_null_builder() {
        let heartbeat_monitor_builder_create_result = heartbeat_monitor_builder_create(100, 200, null_mut());
        assert_eq!(heartbeat_monitor_builder_create_result, FFICode::NullParameter);
    }

    #[test]
    fn heartbeat_monitor_builder_destroy_null_builder() {
        let heartbeat_monitor_builder_destroy_result = heartbeat_monitor_builder_destroy(null_mut());
        assert_eq!(heartbeat_monitor_builder_destroy_result, FFICode::NullParameter);
    }

    #[test]
    fn heartbeat_monitor_destroy_null_monitor() {
        let heartbeat_monitor_destroy_result = heartbeat_monitor_destroy(null_mut());
        assert_eq!(heartbeat_monitor_destroy_result, FFICode::NullParameter);
    }

    #[test]
    fn heartbeat_monitor_heartbeat_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_builder_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_handle: FFIHandle = null_mut();

        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let _ = heartbeat_monitor_builder_create(100, 200, &mut heartbeat_monitor_builder_handle as *mut FFIHandle);
        let _ = health_monitor_builder_add_heartbeat_monitor(
            health_monitor_builder_handle,
            &heartbeat_monitor_tag as *const MonitorTag,
            heartbeat_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );
        let _ = health_monitor_get_heartbeat_monitor(
            health_monitor_handle,
            &heartbeat_monitor_tag as *const MonitorTag,
            &mut heartbeat_monitor_handle as *mut FFIHandle,
        );

        let heartbeat_monitor_heartbeat_result = heartbeat_monitor_heartbeat(heartbeat_monitor_handle);
        assert_eq!(heartbeat_monitor_heartbeat_result, FFICode::Success);

        // Clean-up.
        heartbeat_monitor_destroy(heartbeat_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn heartbeat_monitor_heartbeat_null_monitor() {
        let heartbeat_monitor_heartbeat_result = heartbeat_monitor_heartbeat(null_mut());
        assert_eq!(heartbeat_monitor_heartbeat_result, FFICode::NullParameter);
    }
}
