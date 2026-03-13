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

use crate::ffi::{FFIBorrowed, FFICode, FFIHandle};
use crate::logic::{LogicMonitor, LogicMonitorBuilder};
use crate::tag::StateTag;

#[unsafe(no_mangle)]
pub extern "C" fn logic_monitor_builder_create(
    initial_state: *const StateTag,
    logic_monitor_builder_handle_out: *mut FFIHandle,
) -> FFICode {
    if initial_state.is_null() || logic_monitor_builder_handle_out.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // `StateTag` type must be compatible between C++ and Rust.
    let initial_state = unsafe { *initial_state };

    let logic_monitor_builder = LogicMonitorBuilder::new(initial_state);
    unsafe {
        *logic_monitor_builder_handle_out = Box::into_raw(Box::new(logic_monitor_builder)).cast();
    }

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn logic_monitor_builder_destroy(logic_monitor_builder_handle: FFIHandle) -> FFICode {
    if logic_monitor_builder_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `logic_monitor_builder_create`.
    unsafe {
        let _ = Box::from_raw(logic_monitor_builder_handle as *mut LogicMonitorBuilder);
    }

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn logic_monitor_builder_add_state(
    logic_monitor_builder_handle: FFIHandle,
    state: *const StateTag,
    allowed_states: *const StateTag,
    num_allowed_states: usize,
) -> FFICode {
    if logic_monitor_builder_handle.is_null() || state.is_null() {
        return FFICode::NullParameter;
    }
    // Null is only allowed when `num_allowed_states` equals 0!
    if allowed_states.is_null() && num_allowed_states > 0 {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // `StateTag` type must be compatible between C++ and Rust.
    let state = unsafe { *state };

    // SAFETY:
    // `allowed_states` must contain a valid continuous array.
    // Number of elements must match `num_allowed_states`.
    // Null is allowed when `num_allowed_states` equals 0.
    let allowed_states = if num_allowed_states > 0 {
        unsafe { core::slice::from_raw_parts(allowed_states, num_allowed_states) }
    } else {
        &[]
    };

    // SAFETY:
    // Validity of this pointer is ensured.
    // It is assumed that the pointer was created by a call to `logic_monitor_builder_create`.
    // It is assumed that the pointer was not consumed by a call to `logic_monitor_builder_destroy`.
    let mut logic_monitor_builder =
        FFIBorrowed::new(unsafe { Box::from_raw(logic_monitor_builder_handle as *mut LogicMonitorBuilder) });

    logic_monitor_builder.add_state_internal(state, allowed_states);

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn logic_monitor_destroy(logic_monitor_handle: FFIHandle) -> FFICode {
    if logic_monitor_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_get_logic_monitor`.
    unsafe {
        let _ = Box::from_raw(logic_monitor_handle as *mut LogicMonitor);
    }

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn logic_monitor_transition(logic_monitor_handle: FFIHandle, target_state: *const StateTag) -> FFICode {
    if logic_monitor_handle.is_null() || target_state.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // `StateTag` type must be compatible between C++ and Rust.
    let target_state = unsafe { *target_state };

    // SAFETY:
    // Validity of this pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_get_logic_monitor`.
    // It is assumed that the pointer was not consumed by a call to `logic_monitor_destroy`.
    let monitor = FFIBorrowed::new(unsafe { Box::from_raw(logic_monitor_handle as *mut LogicMonitor) });

    // `transition` method returns new state tag on success.
    // This can be handled in C++ layer.
    match monitor.transition(target_state) {
        Ok(_) => FFICode::Success,
        Err(_) => FFICode::Failed,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn logic_monitor_state(logic_monitor_handle: FFIHandle, state_out: *mut StateTag) -> FFICode {
    if logic_monitor_handle.is_null() || state_out.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of this pointer is ensured.
    // It is assumed that the pointer was created by a call to `health_monitor_get_logic_monitor`.
    // It is assumed that the pointer was not consumed by a call to `logic_monitor_destroy`.
    let monitor = FFIBorrowed::new(unsafe { Box::from_raw(logic_monitor_handle as *mut LogicMonitor) });

    match monitor.state() {
        Ok(state) => {
            unsafe {
                *state_out = state;
            }
            FFICode::Success
        },
        Err(_) => FFICode::Failed,
    }
}

#[score_testing_macros::test_mod_with_log]
#[cfg(all(test, not(loom)))]
mod tests {
    use crate::ffi::{
        health_monitor_builder_add_logic_monitor, health_monitor_builder_build, health_monitor_builder_create,
        health_monitor_destroy, health_monitor_get_logic_monitor, FFICode, FFIHandle,
    };
    use crate::logic::ffi::{
        logic_monitor_builder_add_state, logic_monitor_builder_create, logic_monitor_builder_destroy,
        logic_monitor_destroy, logic_monitor_state, logic_monitor_transition,
    };
    use crate::tag::{MonitorTag, StateTag};
    use core::mem::MaybeUninit;
    use core::ptr::null_mut;

    #[test]
    fn logic_monitor_builder_create_succeeds() {
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();

        let initial_state = StateTag::from("initial_state");
        let logic_monitor_builder_create_result = logic_monitor_builder_create(
            &initial_state as *const StateTag,
            &mut logic_monitor_builder_handle as *mut FFIHandle,
        );
        assert!(!logic_monitor_builder_handle.is_null());
        assert_eq!(logic_monitor_builder_create_result, FFICode::Success);

        // Clean-up.
        // NOTE: `logic_monitor_builder_destroy` positive path is already tested here.
        let logic_monitor_builder_destroy_result = logic_monitor_builder_destroy(logic_monitor_builder_handle);
        assert_eq!(logic_monitor_builder_destroy_result, FFICode::Success);
    }

    #[test]
    fn logic_monitor_builder_create_null_builder() {
        let initial_state = StateTag::from("initial_state");
        let logic_monitor_builder_create_result =
            logic_monitor_builder_create(&initial_state as *const StateTag, null_mut());
        assert_eq!(logic_monitor_builder_create_result, FFICode::NullParameter);
    }

    #[test]
    fn logic_monitor_builder_create_null_initial_state() {
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();

        let logic_monitor_builder_create_result =
            logic_monitor_builder_create(null_mut(), &mut logic_monitor_builder_handle as *mut FFIHandle);
        assert_eq!(logic_monitor_builder_create_result, FFICode::NullParameter);

        // Clean-up.
        logic_monitor_builder_destroy(logic_monitor_builder_handle);
    }

    #[test]
    fn logic_monitor_builder_destroy_null_builder() {
        let logic_monitor_builder_destroy_result = logic_monitor_builder_destroy(null_mut());
        assert_eq!(logic_monitor_builder_destroy_result, FFICode::NullParameter);
    }

    #[test]
    fn logic_monitor_builder_add_state_succeeds() {
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();

        let state1 = StateTag::from("state1");
        let _ = logic_monitor_builder_create(
            &state1 as *const StateTag,
            &mut logic_monitor_builder_handle as *mut FFIHandle,
        );

        let state2 = StateTag::from("state2");
        let allowed_states = [state2];
        let result = logic_monitor_builder_add_state(
            logic_monitor_builder_handle,
            &state1 as *const StateTag,
            allowed_states.as_ptr(),
            allowed_states.len(),
        );
        assert_eq!(result, FFICode::Success);

        // Clean-up.
        logic_monitor_builder_destroy(logic_monitor_builder_handle);
    }

    #[test]
    fn logic_monitor_builder_add_state_null_builder() {
        let state = StateTag::from("state");
        let allowed_states = [];
        let result = logic_monitor_builder_add_state(
            null_mut(),
            &state as *const StateTag,
            allowed_states.as_ptr(),
            allowed_states.len(),
        );
        assert_eq!(result, FFICode::NullParameter);
    }

    #[test]
    fn logic_monitor_builder_add_state_null_tag() {
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();

        let state1 = StateTag::from("state1");
        let _ = logic_monitor_builder_create(
            &state1 as *const StateTag,
            &mut logic_monitor_builder_handle as *mut FFIHandle,
        );

        let allowed_states = [];
        let result = logic_monitor_builder_add_state(
            logic_monitor_builder_handle,
            null_mut(),
            allowed_states.as_ptr(),
            allowed_states.len(),
        );
        assert_eq!(result, FFICode::NullParameter);

        // Clean-up.
        logic_monitor_builder_destroy(logic_monitor_builder_handle);
    }

    #[test]
    fn logic_monitor_builder_add_state_null_allowed_states_zero_elements() {
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();

        let state1 = StateTag::from("state1");
        let _ = logic_monitor_builder_create(
            &state1 as *const StateTag,
            &mut logic_monitor_builder_handle as *mut FFIHandle,
        );

        let result =
            logic_monitor_builder_add_state(logic_monitor_builder_handle, &state1 as *const StateTag, null_mut(), 0);
        assert_eq!(result, FFICode::Success);

        // Clean-up.
        logic_monitor_builder_destroy(logic_monitor_builder_handle);
    }

    #[test]
    fn logic_monitor_builder_add_state_null_allowed_states_many_elements() {
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();

        let state1 = StateTag::from("state1");
        let _ = logic_monitor_builder_create(
            &state1 as *const StateTag,
            &mut logic_monitor_builder_handle as *mut FFIHandle,
        );

        let result =
            logic_monitor_builder_add_state(logic_monitor_builder_handle, &state1 as *const StateTag, null_mut(), 2);
        assert_eq!(result, FFICode::NullParameter);

        // Clean-up.
        logic_monitor_builder_destroy(logic_monitor_builder_handle);
    }

    #[test]
    fn logic_monitor_destroy_null_monitor() {
        let logic_monitor_destroy_result = logic_monitor_destroy(null_mut());
        assert_eq!(logic_monitor_destroy_result, FFICode::NullParameter);
    }

    #[test]
    fn logic_monitor_transition_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();
        let mut logic_monitor_handle: FFIHandle = null_mut();

        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
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
        let _ = health_monitor_get_logic_monitor(
            health_monitor_handle,
            &logic_monitor_tag as *const MonitorTag,
            &mut logic_monitor_handle as *mut FFIHandle,
        );

        let result = logic_monitor_transition(logic_monitor_handle, &state2 as *const StateTag);
        assert_eq!(result, FFICode::Success);

        // Clean-up.
        logic_monitor_destroy(logic_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn logic_monitor_transition_fails() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();
        let mut logic_monitor_handle: FFIHandle = null_mut();

        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
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
        let _ = health_monitor_get_logic_monitor(
            health_monitor_handle,
            &logic_monitor_tag as *const MonitorTag,
            &mut logic_monitor_handle as *mut FFIHandle,
        );

        let state3 = StateTag::from("state3");
        let result = logic_monitor_transition(logic_monitor_handle, &state3 as *const StateTag);
        assert_eq!(result, FFICode::Failed);

        // Clean-up.
        logic_monitor_destroy(logic_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn logic_monitor_transition_null_monitor() {
        let state1 = StateTag::from("state1");
        let result = logic_monitor_transition(null_mut(), &state1 as *const StateTag);
        assert_eq!(result, FFICode::NullParameter);
    }

    #[test]
    fn logic_monitor_transition_null_target_state() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();
        let mut logic_monitor_handle: FFIHandle = null_mut();

        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
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
        let _ = health_monitor_get_logic_monitor(
            health_monitor_handle,
            &logic_monitor_tag as *const MonitorTag,
            &mut logic_monitor_handle as *mut FFIHandle,
        );

        let result = logic_monitor_transition(logic_monitor_handle, null_mut());
        assert_eq!(result, FFICode::NullParameter);

        // Clean-up.
        logic_monitor_destroy(logic_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn logic_monitor_state_succeeds() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();
        let mut logic_monitor_handle: FFIHandle = null_mut();

        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
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
        let _ = health_monitor_get_logic_monitor(
            health_monitor_handle,
            &logic_monitor_tag as *const MonitorTag,
            &mut logic_monitor_handle as *mut FFIHandle,
        );

        let mut current_state = MaybeUninit::uninit();
        let result = logic_monitor_state(logic_monitor_handle, current_state.as_mut_ptr());
        assert_eq!(result, FFICode::Success);
        assert_eq!(unsafe { current_state.assume_init() }, state1);

        // Clean-up.
        logic_monitor_destroy(logic_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn logic_monitor_state_fails() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();
        let mut logic_monitor_handle: FFIHandle = null_mut();

        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
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
        let _ = health_monitor_get_logic_monitor(
            health_monitor_handle,
            &logic_monitor_tag as *const MonitorTag,
            &mut logic_monitor_handle as *mut FFIHandle,
        );

        let state3 = StateTag::from("state3");
        let _ = logic_monitor_transition(logic_monitor_handle, &state3 as *const StateTag);

        let mut current_state = MaybeUninit::uninit();
        let result = logic_monitor_state(logic_monitor_handle, current_state.as_mut_ptr());
        assert_eq!(result, FFICode::Failed);

        // Clean-up.
        logic_monitor_destroy(logic_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }

    #[test]
    fn logic_monitor_state_null_monitor() {
        let mut current_state = MaybeUninit::uninit();
        let result = logic_monitor_state(null_mut(), current_state.as_mut_ptr());
        assert_eq!(result, FFICode::NullParameter);
    }

    #[test]
    fn logic_monitor_state_null_state() {
        let mut health_monitor_builder_handle: FFIHandle = null_mut();
        let mut health_monitor_handle: FFIHandle = null_mut();
        let mut logic_monitor_builder_handle: FFIHandle = null_mut();
        let mut logic_monitor_handle: FFIHandle = null_mut();

        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let _ = health_monitor_builder_create(&mut health_monitor_builder_handle as *mut FFIHandle);
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
        let _ = health_monitor_get_logic_monitor(
            health_monitor_handle,
            &logic_monitor_tag as *const MonitorTag,
            &mut logic_monitor_handle as *mut FFIHandle,
        );

        let result = logic_monitor_state(logic_monitor_handle, null_mut());
        assert_eq!(result, FFICode::NullParameter);

        // Clean-up.
        logic_monitor_destroy(logic_monitor_handle);
        health_monitor_destroy(health_monitor_handle);
    }
}
