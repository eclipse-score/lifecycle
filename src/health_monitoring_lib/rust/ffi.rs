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
use crate::deadline::DeadlineMonitorBuilder;
use crate::health_monitor::{HealthMonitor, HealthMonitorBuilder, HealthMonitorError};
use crate::heartbeat::HeartbeatMonitorBuilder;
use crate::logic::LogicMonitorBuilder;
use crate::tag::MonitorTag;
use core::mem::ManuallyDrop;
use core::ops::{Deref, DerefMut};
use core::time::Duration;
use score_log::ScoreDebug;

pub type FFIHandle = *mut core::ffi::c_void;

/// FFI return codes.
/// Must be aligned with `score::hm::Error` with additional success value.
#[repr(u8)]
#[allow(dead_code)]
#[derive(PartialEq, Eq, Debug, ScoreDebug)]
pub enum FFICode {
    Success = 0,
    NullParameter,
    NotFound,
    AlreadyExists,
    InvalidArgument,
    WrongState,
    Failed,
}

impl From<HealthMonitorError> for FFICode {
    fn from(value: HealthMonitorError) -> Self {
        match value {
            HealthMonitorError::NotFound => FFICode::NotFound,
            HealthMonitorError::InvalidArgument => FFICode::InvalidArgument,
            HealthMonitorError::WrongState => FFICode::WrongState,
        }
    }
}

/// A wrapper to represent borrowed data over FFI boundary without taking ownership.
pub struct FFIBorrowed<T> {
    data: ManuallyDrop<T>,
}

impl<T> FFIBorrowed<T> {
    pub fn new(data: T) -> Self {
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

#[unsafe(no_mangle)]
pub extern "C" fn health_monitor_builder_create(health_monitor_builder_handle_out: *mut FFIHandle) -> FFICode {
    if health_monitor_builder_handle_out.is_null() {
        return FFICode::NullParameter;
    }

    let health_monitor_builder = HealthMonitorBuilder::new();
    unsafe {
        *health_monitor_builder_handle_out = Box::into_raw(Box::new(health_monitor_builder)).cast();
    }

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn health_monitor_builder_destroy(health_monitor_builder_handle: FFIHandle) -> FFICode {
    if health_monitor_builder_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_builder_create`.
    // It is assumed that the pointer was not consumed by a call to `health_monitor_builder_build`.
    unsafe {
        let _ = Box::from_raw(health_monitor_builder_handle as *mut HealthMonitorBuilder);
    }

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn health_monitor_builder_build(
    health_monitor_builder_handle: FFIHandle,
    supervisor_cycle_ms: u32,
    internal_cycle_ms: u32,
    health_monitor_handle_out: *mut FFIHandle,
) -> FFICode {
    if health_monitor_builder_handle.is_null() || health_monitor_handle_out.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_builder_create`.
    // It is assumed that the pointer was not consumed by a call to `health_monitor_builder_destroy`.
    let mut health_monitor_builder =
        unsafe { Box::from_raw(health_monitor_builder_handle as *mut HealthMonitorBuilder) };

    health_monitor_builder.with_internal_processing_cycle_internal(Duration::from_millis(internal_cycle_ms as u64));
    health_monitor_builder.with_supervisor_api_cycle_internal(Duration::from_millis(supervisor_cycle_ms as u64));

    // Build instance.
    match health_monitor_builder.build() {
        Ok(health_monitor) => {
            unsafe {
                *health_monitor_handle_out = Box::into_raw(Box::new(health_monitor)).cast();
            }
            FFICode::Success
        },
        Err(e) => e.into(),
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn health_monitor_builder_add_deadline_monitor(
    health_monitor_builder_handle: FFIHandle,
    monitor_tag: *const MonitorTag,
    deadline_monitor_builder_handle: FFIHandle,
) -> FFICode {
    if health_monitor_builder_handle.is_null() || monitor_tag.is_null() || deadline_monitor_builder_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // `MonitorTag` type must be compatible between C++ and Rust.
    let monitor_tag = unsafe { *monitor_tag };

    // SAFETY:
    // Validity of this pointer is ensured.
    // It is assumed that the pointer was created by a call to `deadline_monitor_builder_create`.
    // It is assumed that the pointer was not consumed by a call to `deadline_monitor_builder_destroy`.
    let deadline_monitor_builder =
        unsafe { Box::from_raw(deadline_monitor_builder_handle as *mut DeadlineMonitorBuilder) };

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_builder_create`.
    // It is assumed that the pointer was not consumed by calls to `health_monitor_builder_destroy` or `health_monitor_builder_build`.
    let mut health_monitor_builder =
        FFIBorrowed::new(unsafe { Box::from_raw(health_monitor_builder_handle as *mut HealthMonitorBuilder) });

    health_monitor_builder.add_deadline_monitor_internal(monitor_tag, *deadline_monitor_builder);

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn health_monitor_builder_add_heartbeat_monitor(
    health_monitor_builder_handle: FFIHandle,
    monitor_tag: *const MonitorTag,
    heartbeat_monitor_builder_handle: FFIHandle,
) -> FFICode {
    if health_monitor_builder_handle.is_null() || monitor_tag.is_null() || heartbeat_monitor_builder_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // `MonitorTag` type must be compatible between C++ and Rust.
    let monitor_tag = unsafe { *monitor_tag };

    // SAFETY:
    // Validity of this pointer is ensured.
    // It is assumed that the pointer was created by a call to `heartbeat_monitor_builder_create`.
    // It is assumed that the pointer was not consumed by a call to `heartbeat_monitor_builder_destroy`.
    let heartbeat_monitor_builder =
        unsafe { Box::from_raw(heartbeat_monitor_builder_handle as *mut HeartbeatMonitorBuilder) };

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_builder_create`.
    // It is assumed that the pointer was not consumed by calls to `health_monitor_builder_destroy` or `health_monitor_builder_build`.
    let mut health_monitor_builder =
        FFIBorrowed::new(unsafe { Box::from_raw(health_monitor_builder_handle as *mut HealthMonitorBuilder) });

    health_monitor_builder.add_heartbeat_monitor_internal(monitor_tag, *heartbeat_monitor_builder);

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn health_monitor_builder_add_logic_monitor(
    health_monitor_builder_handle: FFIHandle,
    monitor_tag: *const MonitorTag,
    logic_monitor_builder_handle: FFIHandle,
) -> FFICode {
    if health_monitor_builder_handle.is_null() || monitor_tag.is_null() || logic_monitor_builder_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // `MonitorTag` type must be compatible between C++ and Rust.
    let monitor_tag = unsafe { *monitor_tag };

    // SAFETY:
    // Validity of this pointer is ensured.
    // It is assumed that the pointer was created by a call to `logic_monitor_builder_create`.
    // It is assumed that the pointer was not consumed by a call to `logic_monitor_builder_destroy`.
    let logic_monitor_builder = unsafe { Box::from_raw(logic_monitor_builder_handle as *mut LogicMonitorBuilder) };

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_builder_create`.
    // It is assumed that the pointer was not consumed by calls to `health_monitor_builder_destroy` or `health_monitor_builder_build`.
    let mut health_monitor_builder =
        FFIBorrowed::new(unsafe { Box::from_raw(health_monitor_builder_handle as *mut HealthMonitorBuilder) });

    health_monitor_builder.add_logic_monitor_internal(monitor_tag, *logic_monitor_builder);

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn health_monitor_get_deadline_monitor(
    health_monitor_handle: FFIHandle,
    monitor_tag: *const MonitorTag,
    deadline_monitor_handle_out: *mut FFIHandle,
) -> FFICode {
    if health_monitor_handle.is_null() || monitor_tag.is_null() || deadline_monitor_handle_out.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // `MonitorTag` type must be compatible between C++ and Rust.
    let monitor_tag = unsafe { *monitor_tag };

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_builder_build`.
    // It is assumed that the pointer was not consumed by a call to `health_monitor_destroy`.
    let mut health_monitor = FFIBorrowed::new(unsafe { Box::from_raw(health_monitor_handle as *mut HealthMonitor) });

    if let Some(deadline_monitor) = health_monitor.get_deadline_monitor(monitor_tag) {
        unsafe {
            *deadline_monitor_handle_out = Box::into_raw(Box::new(DeadlineMonitorCpp::new(deadline_monitor))).cast();
        }
        FFICode::Success
    } else {
        FFICode::NotFound
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn health_monitor_get_heartbeat_monitor(
    health_monitor_handle: FFIHandle,
    monitor_tag: *const MonitorTag,
    heartbeat_monitor_handle_out: *mut FFIHandle,
) -> FFICode {
    if health_monitor_handle.is_null() || monitor_tag.is_null() || heartbeat_monitor_handle_out.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // `MonitorTag` type must be compatible between C++ and Rust.
    let monitor_tag = unsafe { *monitor_tag };

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_builder_build`.
    // It is assumed that the pointer was not consumed by a call to `health_monitor_destroy`.
    let mut health_monitor = FFIBorrowed::new(unsafe { Box::from_raw(health_monitor_handle as *mut HealthMonitor) });

    if let Some(heartbeat_monitor) = health_monitor.get_heartbeat_monitor(monitor_tag) {
        unsafe {
            *heartbeat_monitor_handle_out = Box::into_raw(Box::new(heartbeat_monitor)).cast();
        }
        FFICode::Success
    } else {
        FFICode::NotFound
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn health_monitor_get_logic_monitor(
    health_monitor_handle: FFIHandle,
    monitor_tag: *const MonitorTag,
    logic_monitor_handle_out: *mut FFIHandle,
) -> FFICode {
    if health_monitor_handle.is_null() || monitor_tag.is_null() || logic_monitor_handle_out.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // `MonitorTag` type must be compatible between C++ and Rust.
    let monitor_tag = unsafe { *monitor_tag };

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_builder_build`.
    // It is assumed that the pointer was not consumed by a call to `health_monitor_destroy`.
    let mut health_monitor = FFIBorrowed::new(unsafe { Box::from_raw(health_monitor_handle as *mut HealthMonitor) });

    if let Some(logic_monitor) = health_monitor.get_logic_monitor(monitor_tag) {
        unsafe {
            *logic_monitor_handle_out = Box::into_raw(Box::new(logic_monitor)).cast();
        }
        FFICode::Success
    } else {
        FFICode::NotFound
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn health_monitor_start(health_monitor_handle: FFIHandle) -> FFICode {
    if health_monitor_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_builder_build`.
    // It is assumed that the pointer was not consumed by a call to `health_monitor_destroy`.
    let mut health_monitor = FFIBorrowed::new(unsafe { Box::from_raw(health_monitor_handle as *mut HealthMonitor) });

    // Start monitoring logic.
    match health_monitor.start() {
        Ok(_) => FFICode::Success,
        Err(error) => error.into(),
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn health_monitor_destroy(health_monitor_handle: FFIHandle) -> FFICode {
    if health_monitor_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_builder_build`.
    unsafe {
        let _ = Box::from_raw(health_monitor_handle as *mut HealthMonitor);
    }

    FFICode::Success
}

#[score_testing_macros::test_mod_with_log]
#[cfg(all(test, not(loom)))]
mod tests {
    use crate::deadline::ffi::{
        deadline_monitor_builder_create, deadline_monitor_builder_destroy, deadline_monitor_destroy,
    };
    use crate::ffi::{
        health_monitor_builder_add_deadline_monitor, health_monitor_builder_add_heartbeat_monitor,
        health_monitor_builder_add_logic_monitor, health_monitor_builder_build, health_monitor_builder_create,
        health_monitor_builder_destroy, health_monitor_destroy, health_monitor_get_deadline_monitor,
        health_monitor_get_heartbeat_monitor, health_monitor_get_logic_monitor, health_monitor_start, FFICode,
        FFIHandle,
    };
    use crate::heartbeat::ffi::{
        heartbeat_monitor_builder_create, heartbeat_monitor_builder_destroy, heartbeat_monitor_destroy,
    };
    use crate::logic::ffi::{
        logic_monitor_builder_add_state, logic_monitor_builder_create, logic_monitor_builder_destroy,
        logic_monitor_destroy,
    };
    use crate::tag::{MonitorTag, StateTag};
    use core::ptr::null_mut;

    fn def_logic_monitor_builder() -> FFIHandle {
        let mut logic_monitor_builder_handle = null_mut();
        let state1 = StateTag::from("state1");
        let state2 = StateTag::from("state2");
        let _ = logic_monitor_builder_create(
            &state1 as *const StateTag,
            &mut logic_monitor_builder_handle as *mut FFIHandle,
        );

        let _ = logic_monitor_builder_add_state(
            logic_monitor_builder_handle,
            &state1 as *const StateTag,
            [state2].as_ptr(),
            1,
        );
        let _ =
            logic_monitor_builder_add_state(logic_monitor_builder_handle, &state2 as *const StateTag, [].as_ptr(), 0);

        logic_monitor_builder_handle
    }

    #[test]
    fn health_monitor_builder_create_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();

        let health_monitor_builder_create_result =
            health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        assert!(!health_monitor_builder_handle.is_null());
        assert_eq!(health_monitor_builder_create_result, FFICode::Success);

        // Clean-up.
        // NOTE: `health_monitor_builder_destroy` positive path is already tested here.
        let health_monitor_builder_destroy_result = health_monitor_builder_destroy(health_monitor_builder_handle);
        assert_eq!(health_monitor_builder_destroy_result, FFICode::Success);
    }

    #[test]
    fn health_monitor_builder_create_null_handle() {
        let health_monitor_builder_create_result = health_monitor_builder_create(null_mut());
        assert_eq!(health_monitor_builder_create_result, FFICode::NullParameter);
    }

    #[test]
    fn health_monitor_builder_destroy_null_handle() {
        let health_monitor_builder_destroy_result = health_monitor_builder_destroy(null_mut());
        assert_eq!(health_monitor_builder_destroy_result, FFICode::NullParameter);
    }

    #[test]
    fn health_monitor_builder_build_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const MonitorTag,
            deadline_monitor_builder_handle,
        );

        let health_monitor_builder_build_result = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );
        assert!(!health_monitor_handle.is_null());
        assert_eq!(health_monitor_builder_build_result, FFICode::Success);

        // Clean-up.
        // NOTE: `health_monitor_destroy` positive path is already tested here.
        let health_monitor_destroy_result = health_monitor_destroy(health_monitor_handle);
        assert_eq!(health_monitor_destroy_result, FFICode::Success);
    }

    #[test]
    fn health_monitor_builder_build_invalid_cycle_intervals() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);

        let health_monitor_builder_build_result = health_monitor_builder_build(
            health_monitor_builder_handle,
            123,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );
        assert!(health_monitor_handle.is_null());
        assert_eq!(health_monitor_builder_build_result, FFICode::InvalidArgument);

        // Clean-up not needed - health monitor builder was already consumed by the `build`.
    }

    #[test]
    fn health_monitor_builder_build_no_monitors() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);

        let health_monitor_builder_build_result = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );
        assert_eq!(health_monitor_builder_build_result, FFICode::WrongState);

        // Clean-up not needed - health monitor builder was already consumed by the `build`.
    }

    #[test]
    fn health_monitor_builder_build_null_builder_handle() {
        let mut health_monitor_handle: FFIHandle = null_mut();

        let health_monitor_builder_build_result =
            health_monitor_builder_build(null_mut(), 200, 100, &mut health_monitor_handle as *mut FFIHandle);
        assert!(health_monitor_handle.is_null());
        assert_eq!(health_monitor_builder_build_result, FFICode::NullParameter);
    }

    #[test]
    fn health_monitor_builder_build_null_monitor_handle() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);

        let health_monitor_builder_build_result =
            health_monitor_builder_build(health_monitor_builder_handle, 200, 100, null_mut());
        assert_eq!(health_monitor_builder_build_result, FFICode::NullParameter);

        // Clean-up.
        health_monitor_builder_destroy(health_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_deadline_monitor_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);

        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);

        let health_monitor_builder_add_deadline_monitor_result = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const MonitorTag,
            deadline_monitor_builder_handle,
        );
        assert_eq!(health_monitor_builder_add_deadline_monitor_result, FFICode::Success);

        // Clean-up.
        health_monitor_builder_destroy(health_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_deadline_monitor_null_hmon_builder() {
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();

        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);

        let health_monitor_builder_add_deadline_monitor_result = health_monitor_builder_add_deadline_monitor(
            null_mut(),
            &deadline_monitor_tag as *const MonitorTag,
            deadline_monitor_builder_handle,
        );
        assert_eq!(
            health_monitor_builder_add_deadline_monitor_result,
            FFICode::NullParameter
        );

        // Clean-up.
        deadline_monitor_builder_destroy(deadline_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_deadline_monitor_null_monitor_tag() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);

        let health_monitor_builder_add_deadline_monitor_result = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            null_mut(),
            deadline_monitor_builder_handle,
        );
        assert_eq!(
            health_monitor_builder_add_deadline_monitor_result,
            FFICode::NullParameter
        );

        // Clean-up.
        deadline_monitor_builder_destroy(deadline_monitor_builder_handle);
        health_monitor_builder_destroy(health_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_deadline_monitor_null_deadline_monitor_builder() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");

        let health_monitor_builder_add_deadline_monitor_result = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const MonitorTag,
            null_mut(),
        );
        assert_eq!(
            health_monitor_builder_add_deadline_monitor_result,
            FFICode::NullParameter
        );

        // Clean-up.
        health_monitor_builder_destroy(health_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_heartbeat_monitor_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);

        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
        let _ = heartbeat_monitor_builder_create(100, 200, &mut heartbeat_monitor_builder_handle as *mut FFIHandle);

        let health_monitor_builder_add_heartbeat_monitor_result = health_monitor_builder_add_heartbeat_monitor(
            health_monitor_builder_handle,
            &heartbeat_monitor_tag as *const MonitorTag,
            heartbeat_monitor_builder_handle,
        );
        assert_eq!(health_monitor_builder_add_heartbeat_monitor_result, FFICode::Success);

        // Clean-up.
        health_monitor_builder_destroy(health_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_heartbeat_monitor_null_hmon_builder() {
        let mut heartbeat_monitor_builder_handle: FFIHandle = null_mut();

        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
        let _ = heartbeat_monitor_builder_create(100, 200, &mut heartbeat_monitor_builder_handle as *mut FFIHandle);

        let health_monitor_builder_add_heartbeat_monitor_result = health_monitor_builder_add_heartbeat_monitor(
            null_mut(),
            &heartbeat_monitor_tag as *const MonitorTag,
            heartbeat_monitor_builder_handle,
        );
        assert_eq!(
            health_monitor_builder_add_heartbeat_monitor_result,
            FFICode::NullParameter
        );

        // Clean-up.
        heartbeat_monitor_builder_destroy(heartbeat_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_heartbeat_monitor_null_monitor_tag() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let _ = heartbeat_monitor_builder_create(100, 200, &mut heartbeat_monitor_builder_handle as *mut FFIHandle);

        let health_monitor_builder_add_heartbeat_monitor_result = health_monitor_builder_add_heartbeat_monitor(
            health_monitor_builder_handle,
            null_mut(),
            heartbeat_monitor_builder_handle,
        );
        assert_eq!(
            health_monitor_builder_add_heartbeat_monitor_result,
            FFICode::NullParameter
        );

        // Clean-up.
        heartbeat_monitor_builder_destroy(heartbeat_monitor_builder_handle);
        health_monitor_builder_destroy(health_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_heartbeat_monitor_null_heartbeat_monitor_builder() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");

        let health_monitor_builder_add_heartbeat_monitor_result = health_monitor_builder_add_heartbeat_monitor(
            health_monitor_builder_handle,
            &heartbeat_monitor_tag as *const MonitorTag,
            null_mut(),
        );
        assert_eq!(
            health_monitor_builder_add_heartbeat_monitor_result,
            FFICode::NullParameter
        );

        // Clean-up.
        health_monitor_builder_destroy(health_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_logic_monitor_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);

        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let initial_state = StateTag::from("initial_state");
        let _ = logic_monitor_builder_create(
            &initial_state as *const StateTag,
            &mut logic_monitor_builder_handle as *mut FFIHandle,
        );

        let health_monitor_builder_add_logic_monitor_result = health_monitor_builder_add_logic_monitor(
            health_monitor_builder_handle,
            &logic_monitor_tag as *const MonitorTag,
            logic_monitor_builder_handle,
        );
        assert_eq!(health_monitor_builder_add_logic_monitor_result, FFICode::Success);

        // Clean-up.
        health_monitor_builder_destroy(health_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_logic_monitor_null_hmon_builder() {
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();

        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let initial_state = StateTag::from("initial_state");
        let _ = logic_monitor_builder_create(
            &initial_state as *const StateTag,
            &mut logic_monitor_builder_handle as *mut FFIHandle,
        );

        let health_monitor_builder_add_logic_monitor_result = health_monitor_builder_add_logic_monitor(
            null_mut(),
            &logic_monitor_tag as *const MonitorTag,
            logic_monitor_builder_handle,
        );
        assert_eq!(health_monitor_builder_add_logic_monitor_result, FFICode::NullParameter);

        // Clean-up.
        logic_monitor_builder_destroy(logic_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_logic_monitor_null_monitor_tag() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let initial_state = StateTag::from("initial_state");
        let _ = logic_monitor_builder_create(
            &initial_state as *const StateTag,
            &mut logic_monitor_builder_handle as *mut FFIHandle,
        );

        let health_monitor_builder_add_logic_monitor_result = health_monitor_builder_add_logic_monitor(
            health_monitor_builder_handle,
            null_mut(),
            logic_monitor_builder_handle,
        );
        assert_eq!(health_monitor_builder_add_logic_monitor_result, FFICode::NullParameter);

        // Clean-up.
        logic_monitor_builder_destroy(logic_monitor_builder_handle);
        health_monitor_builder_destroy(health_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_builder_add_logic_monitor_null_logic_monitor_builder() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let logic_monitor_tag = MonitorTag::from("logic_monitor");

        let health_monitor_builder_add_logic_monitor_result = health_monitor_builder_add_logic_monitor(
            health_monitor_builder_handle,
            &logic_monitor_tag as *const MonitorTag,
            null_mut(),
        );
        assert_eq!(health_monitor_builder_add_logic_monitor_result, FFICode::NullParameter);

        // Clean-up.
        health_monitor_builder_destroy(health_monitor_builder_handle);
    }

    #[test]
    fn health_monitor_get_deadline_monitor_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const MonitorTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );

        let health_monitor_get_deadline_monitor_result = health_monitor_get_deadline_monitor(
            health_monitor_handle,
            &deadline_monitor_tag as *const MonitorTag,
            &mut deadline_monitor_handle as *mut FFIHandle,
        );
        assert!(!deadline_monitor_handle.is_null());
        assert_eq!(health_monitor_get_deadline_monitor_result, FFICode::Success);

        // Clean-up.
        deadline_monitor_destroy(deadline_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_deadline_monitor_already_taken() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_1_handle: FFIHandle = null_mut();
        let mut deadline_monitor_2_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const MonitorTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );

        // First get.
        let health_monitor_get_deadline_monitor_result_1 = health_monitor_get_deadline_monitor(
            health_monitor_handle,
            &deadline_monitor_tag as *const MonitorTag,
            &mut deadline_monitor_1_handle as *mut FFIHandle,
        );
        assert!(!deadline_monitor_1_handle.is_null());
        assert_eq!(health_monitor_get_deadline_monitor_result_1, FFICode::Success);

        // Second get.
        let health_monitor_get_deadline_monitor_result_2 = health_monitor_get_deadline_monitor(
            health_monitor_handle,
            &deadline_monitor_tag as *const MonitorTag,
            &mut deadline_monitor_2_handle as *mut FFIHandle,
        );
        assert!(deadline_monitor_2_handle.is_null());
        assert_eq!(health_monitor_get_deadline_monitor_result_2, FFICode::NotFound);

        // Clean-up.
        deadline_monitor_destroy(deadline_monitor_1_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_deadline_monitor_null_hmon() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const MonitorTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );

        let health_monitor_get_deadline_monitor_result = health_monitor_get_deadline_monitor(
            null_mut(),
            &deadline_monitor_tag as *const MonitorTag,
            &mut deadline_monitor_handle as *mut FFIHandle,
        );
        assert!(deadline_monitor_handle.is_null());
        assert_eq!(health_monitor_get_deadline_monitor_result, FFICode::NullParameter);

        // Clean-up.
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_deadline_monitor_null_monitor_tag() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const MonitorTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );

        let health_monitor_get_deadline_monitor_result = health_monitor_get_deadline_monitor(
            health_monitor_handle,
            null_mut(),
            &mut deadline_monitor_handle as *mut FFIHandle,
        );
        assert!(deadline_monitor_handle.is_null());
        assert_eq!(health_monitor_get_deadline_monitor_result, FFICode::NullParameter);

        // Clean-up.
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_deadline_monitor_null_deadline_monitor() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const MonitorTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );

        let health_monitor_get_deadline_monitor_result = health_monitor_get_deadline_monitor(
            health_monitor_handle,
            &deadline_monitor_tag as *const MonitorTag,
            null_mut(),
        );
        assert_eq!(health_monitor_get_deadline_monitor_result, FFICode::NullParameter);

        // Clean-up.
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_heartbeat_monitor_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_builder_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
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

        let health_monitor_get_heartbeat_monitor_result = health_monitor_get_heartbeat_monitor(
            health_monitor_handle,
            &heartbeat_monitor_tag as *const MonitorTag,
            &mut heartbeat_monitor_handle as *mut FFIHandle,
        );
        assert!(!heartbeat_monitor_handle.is_null());
        assert_eq!(health_monitor_get_heartbeat_monitor_result, FFICode::Success);

        // Clean-up.
        heartbeat_monitor_destroy(heartbeat_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_heartbeat_monitor_already_taken() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_builder_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_1_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_2_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
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

        // First get.
        let health_monitor_get_heartbeat_monitor_result_1 = health_monitor_get_heartbeat_monitor(
            health_monitor_handle,
            &heartbeat_monitor_tag as *const MonitorTag,
            &mut heartbeat_monitor_1_handle as *mut FFIHandle,
        );
        assert!(!heartbeat_monitor_1_handle.is_null());
        assert_eq!(health_monitor_get_heartbeat_monitor_result_1, FFICode::Success);

        // Second get.
        let health_monitor_get_heartbeat_monitor_result_2 = health_monitor_get_heartbeat_monitor(
            health_monitor_handle,
            &heartbeat_monitor_tag as *const MonitorTag,
            &mut heartbeat_monitor_2_handle as *mut FFIHandle,
        );
        assert!(heartbeat_monitor_2_handle.is_null());
        assert_eq!(health_monitor_get_heartbeat_monitor_result_2, FFICode::NotFound);

        // Clean-up.
        heartbeat_monitor_destroy(heartbeat_monitor_1_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_heartbeat_monitor_null_hmon() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_builder_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
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

        let health_monitor_get_heartbeat_monitor_result = health_monitor_get_heartbeat_monitor(
            null_mut(),
            &heartbeat_monitor_tag as *const MonitorTag,
            &mut heartbeat_monitor_handle as *mut FFIHandle,
        );
        assert!(heartbeat_monitor_handle.is_null());
        assert_eq!(health_monitor_get_heartbeat_monitor_result, FFICode::NullParameter);

        // Clean-up.
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_heartbeat_monitor_null_monitor_tag() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_builder_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
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

        let health_monitor_get_heartbeat_monitor_result = health_monitor_get_heartbeat_monitor(
            health_monitor_handle,
            null_mut(),
            &mut heartbeat_monitor_handle as *mut FFIHandle,
        );
        assert!(heartbeat_monitor_handle.is_null());
        assert_eq!(health_monitor_get_heartbeat_monitor_result, FFICode::NullParameter);

        // Clean-up.
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_heartbeat_monitor_null_heartbeat_monitor() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut heartbeat_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
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

        let health_monitor_get_heartbeat_monitor_result = health_monitor_get_heartbeat_monitor(
            health_monitor_handle,
            &heartbeat_monitor_tag as *const MonitorTag,
            null_mut(),
        );
        assert_eq!(health_monitor_get_heartbeat_monitor_result, FFICode::NullParameter);

        // Clean-up.
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_logic_monitor_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut logic_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let logic_monitor_builder_handle = def_logic_monitor_builder();
        let _ = health_monitor_builder_add_logic_monitor(
            health_monitor_builder_handle,
            &logic_monitor_tag as *const MonitorTag,
            logic_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );

        let health_monitor_get_logic_monitor_result = health_monitor_get_logic_monitor(
            health_monitor_handle,
            &logic_monitor_tag as *const MonitorTag,
            &mut logic_monitor_handle as *mut FFIHandle,
        );
        assert!(!logic_monitor_handle.is_null());
        assert_eq!(health_monitor_get_logic_monitor_result, FFICode::Success);

        // Clean-up.
        logic_monitor_destroy(logic_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_logic_monitor_already_taken() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut logic_monitor_1_handle: FFIHandle = null_mut();
        let mut logic_monitor_2_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let logic_monitor_builder_handle = def_logic_monitor_builder();
        let _ = health_monitor_builder_add_logic_monitor(
            health_monitor_builder_handle,
            &logic_monitor_tag as *const MonitorTag,
            logic_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );

        // First get.
        let health_monitor_get_logic_monitor_result_1 = health_monitor_get_logic_monitor(
            health_monitor_handle,
            &logic_monitor_tag as *const MonitorTag,
            &mut logic_monitor_1_handle as *mut FFIHandle,
        );
        assert!(!logic_monitor_1_handle.is_null());
        assert_eq!(health_monitor_get_logic_monitor_result_1, FFICode::Success);

        // Second get.
        let health_monitor_get_logic_monitor_result_2 = health_monitor_get_logic_monitor(
            health_monitor_handle,
            &logic_monitor_tag as *const MonitorTag,
            &mut logic_monitor_2_handle as *mut FFIHandle,
        );
        assert!(logic_monitor_2_handle.is_null());
        assert_eq!(health_monitor_get_logic_monitor_result_2, FFICode::NotFound);

        // Clean-up.
        logic_monitor_destroy(logic_monitor_1_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_logic_monitor_null_hmon() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut logic_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let logic_monitor_builder_handle = def_logic_monitor_builder();
        let _ = health_monitor_builder_add_logic_monitor(
            health_monitor_builder_handle,
            &logic_monitor_tag as *const MonitorTag,
            logic_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );

        let health_monitor_get_logic_monitor_result = health_monitor_get_logic_monitor(
            null_mut(),
            &logic_monitor_tag as *const MonitorTag,
            &mut logic_monitor_handle as *mut FFIHandle,
        );
        assert!(logic_monitor_handle.is_null());
        assert_eq!(health_monitor_get_logic_monitor_result, FFICode::NullParameter);

        // Clean-up.
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_logic_monitor_null_monitor_tag() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut logic_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let logic_monitor_builder_handle = def_logic_monitor_builder();
        let _ = health_monitor_builder_add_logic_monitor(
            health_monitor_builder_handle,
            &logic_monitor_tag as *const MonitorTag,
            logic_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );

        let health_monitor_get_logic_monitor_result = health_monitor_get_logic_monitor(
            health_monitor_handle,
            null_mut(),
            &mut logic_monitor_handle as *mut FFIHandle,
        );
        assert!(logic_monitor_handle.is_null());
        assert_eq!(health_monitor_get_logic_monitor_result, FFICode::NullParameter);

        // Clean-up.
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_get_logic_monitor_null_logic_monitor() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let logic_monitor_builder_handle = def_logic_monitor_builder();
        let _ = health_monitor_builder_add_logic_monitor(
            health_monitor_builder_handle,
            &logic_monitor_tag as *const MonitorTag,
            logic_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );

        let health_monitor_get_logic_monitor_result = health_monitor_get_logic_monitor(
            health_monitor_handle,
            &logic_monitor_tag as *const MonitorTag,
            null_mut(),
        );
        assert_eq!(health_monitor_get_logic_monitor_result, FFICode::NullParameter);

        // Clean-up.
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_start_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();
        let mut deadline_monitor_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const MonitorTag,
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
            &deadline_monitor_tag as *const MonitorTag,
            &mut deadline_monitor_handle as *mut FFIHandle,
        );

        let health_monitor_start_result = health_monitor_start(health_monitor_handle);
        assert_eq!(health_monitor_start_result, FFICode::Success);

        // Clean-up.
        deadline_monitor_destroy(deadline_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_start_monitor_not_taken() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut deadline_monitor_builder_handle: FFIHandle = null_mut();

        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let _ = deadline_monitor_builder_create(&mut deadline_monitor_builder_handle as *mut FFIHandle);
        let _ = health_monitor_builder_add_deadline_monitor(
            health_monitor_builder_handle,
            &deadline_monitor_tag as *const MonitorTag,
            deadline_monitor_builder_handle,
        );
        let _ = health_monitor_builder_build(
            health_monitor_builder_handle,
            200,
            100,
            &mut health_monitor_handle as *mut FFIHandle,
        );

        let health_monitor_start_result = health_monitor_start(health_monitor_handle);
        assert_eq!(health_monitor_start_result, FFICode::WrongState);

        // Clean-up.
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn health_monitor_start_null_hmon() {
        let health_monitor_start_result = health_monitor_start(null_mut());
        assert_eq!(health_monitor_start_result, FFICode::NullParameter);
    }

    #[test]
    fn health_monitor_destroy_null_hmon() {
        let health_monitor_destroy_result = health_monitor_destroy(null_mut());
        assert_eq!(health_monitor_destroy_result, FFICode::NullParameter);
    }
}
