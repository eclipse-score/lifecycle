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
use thread::{SchedulerParameters, SchedulerPolicy, ThreadParameters};

/// C++ interface proxy for [`ThreadParameters`].
#[derive(Default)]
pub(crate) struct ThreadParametersCpp {
    scheduler_parameters: Option<SchedulerParameters>,
    affinity: Option<Box<[usize]>>,
    stack_size: Option<usize>,
}

impl ThreadParametersCpp {
    fn new() -> Self {
        Self::default()
    }

    fn scheduler_parameters(&mut self, scheduler_parameters: SchedulerParameters) {
        self.scheduler_parameters = Some(scheduler_parameters);
    }

    fn affinity(&mut self, affinity: &[usize]) {
        self.affinity = Some(Box::from(affinity));
    }

    fn stack_size(&mut self, stack_size: usize) {
        self.stack_size = Some(stack_size);
    }

    pub(crate) fn build(self) -> ThreadParameters {
        let mut thread_parameters = ThreadParameters::new();
        if let Some(scheduler_parameters) = self.scheduler_parameters {
            thread_parameters = thread_parameters.scheduler_parameters(scheduler_parameters);
        }
        if let Some(affinity) = self.affinity {
            thread_parameters = thread_parameters.affinity(&affinity);
        }
        if let Some(stack_size) = self.stack_size {
            thread_parameters = thread_parameters.stack_size(stack_size);
        }

        thread_parameters
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn scheduler_policy_priority_min(scheduler_policy: SchedulerPolicy, priority_out: *mut i32) -> FFICode {
    if priority_out.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY: validity of the pointer is ensured.
    unsafe {
        *priority_out = scheduler_policy.priority_min();
    }
    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn scheduler_policy_priority_max(scheduler_policy: SchedulerPolicy, priority_out: *mut i32) -> FFICode {
    if priority_out.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY: validity of the pointer is ensured.
    unsafe {
        *priority_out = scheduler_policy.priority_max();
    }
    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn thread_parameters_create(thread_parameters_handle_out: *mut FFIHandle) -> FFICode {
    if thread_parameters_handle_out.is_null() {
        return FFICode::NullParameter;
    }

    let thread_parameters = ThreadParametersCpp::new();
    unsafe {
        *thread_parameters_handle_out = Box::into_raw(Box::new(thread_parameters)).cast();
    }

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn thread_parameters_destroy(thread_parameters_handle: FFIHandle) -> FFICode {
    if thread_parameters_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `thread_parameters_create`.
    // It is assumed that the pointer was not consumed by a call to `health_monitor_builder_build`.
    unsafe {
        let _ = Box::from_raw(thread_parameters_handle as *mut ThreadParametersCpp);
    }

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn thread_parameters_scheduler_parameters(
    thread_parameters_handle: FFIHandle,
    policy: SchedulerPolicy,
    priority: i32,
) -> FFICode {
    if thread_parameters_handle.is_null() {
        return FFICode::NullParameter;
    }

    // Make sure priority is in allowed range.
    let allowed_priority_range = policy.priority_min()..=policy.priority_max();
    if !allowed_priority_range.contains(&priority) {
        return FFICode::InvalidArgument;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `thread_parameters_create`.
    // It is assumed that the pointer was not consumed by a call to `thread_parameters_destroy` or `health_monitor_builder_build`.
    let mut thread_parameters =
        FFIBorrowed::new(unsafe { Box::from_raw(thread_parameters_handle as *mut ThreadParametersCpp) });

    let scheduler_parameters = SchedulerParameters::new(policy, priority);
    thread_parameters.scheduler_parameters(scheduler_parameters);

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn thread_parameters_affinity(
    thread_parameters_handle: FFIHandle,
    affinity: *const usize,
    num_affinity: usize,
) -> FFICode {
    if thread_parameters_handle.is_null() {
        return FFICode::NullParameter;
    }
    // Null is only allowed when `num_affinity` equals 0!
    if affinity.is_null() && num_affinity > 0 {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // `affinity` must contain a valid continuous array.
    // Number of elements must match `num_affinity`.
    // Null is allowed when `num_affinity` equals 0.
    let affinity = if num_affinity > 0 {
        unsafe { core::slice::from_raw_parts(affinity, num_affinity) }
    } else {
        &[]
    };

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `thread_parameters_create`.
    // It is assumed that the pointer was not consumed by a call to `thread_parameters_destroy` or `health_monitor_builder_build`.
    let mut thread_parameters =
        FFIBorrowed::new(unsafe { Box::from_raw(thread_parameters_handle as *mut ThreadParametersCpp) });

    thread_parameters.affinity(affinity);

    FFICode::Success
}

#[unsafe(no_mangle)]
pub extern "C" fn thread_parameters_stack_size(thread_parameters_handle: FFIHandle, stack_size: usize) -> FFICode {
    if thread_parameters_handle.is_null() {
        return FFICode::NullParameter;
    }

    // SAFETY:
    // Validity of the pointer is ensured.
    // It is assumed that the pointer was created by a call to `thread_parameters_create`.
    // It is assumed that the pointer was not consumed by a call to `thread_parameters_destroy` or `health_monitor_builder_build`.
    let mut thread_parameters =
        FFIBorrowed::new(unsafe { Box::from_raw(thread_parameters_handle as *mut ThreadParametersCpp) });

    thread_parameters.stack_size(stack_size);

    FFICode::Success
}

#[score_testing_macros::test_mod_with_log]
#[cfg(all(test, not(loom)))]
mod tests {
    use crate::ffi::{FFICode, FFIHandle};
    use crate::thread_ffi::{
        scheduler_policy_priority_max, scheduler_policy_priority_min, thread_parameters_affinity,
        thread_parameters_create, thread_parameters_destroy, thread_parameters_scheduler_parameters,
        thread_parameters_stack_size,
    };
    use core::mem::MaybeUninit;
    use core::ptr::null_mut;
    use thread::SchedulerPolicy;

    #[test]
    #[cfg_attr(miri, ignore)]
    fn scheduler_policy_priority_min_succeeds() {
        let policy = SchedulerPolicy::Fifo;
        let mut priority = MaybeUninit::uninit();
        let scheduler_policy_priority_min_result = scheduler_policy_priority_min(policy, priority.as_mut_ptr());
        assert_eq!(scheduler_policy_priority_min_result, FFICode::Success);
        assert_eq!(unsafe { priority.assume_init() }, 1);
    }

    #[test]
    fn scheduler_policy_priority_min_null_priority() {
        let policy = SchedulerPolicy::Fifo;
        let priority = null_mut();
        let scheduler_policy_priority_min_result = scheduler_policy_priority_min(policy, priority);
        assert_eq!(scheduler_policy_priority_min_result, FFICode::NullParameter);
    }

    #[test]
    #[cfg_attr(miri, ignore)]
    fn scheduler_policy_priority_max_succeeds() {
        let policy = SchedulerPolicy::Fifo;
        let mut priority = MaybeUninit::uninit();
        let scheduler_policy_priority_max_result = scheduler_policy_priority_max(policy, priority.as_mut_ptr());
        assert_eq!(scheduler_policy_priority_max_result, FFICode::Success);
        assert_eq!(unsafe { priority.assume_init() }, 99);
    }

    #[test]
    fn scheduler_policy_priority_max_null_priority() {
        let policy = SchedulerPolicy::Fifo;
        let priority = null_mut();
        let scheduler_policy_priority_max_result = scheduler_policy_priority_max(policy, priority);
        assert_eq!(scheduler_policy_priority_max_result, FFICode::NullParameter);
    }

    #[test]
    fn thread_parameters_create_succeeds() {
        let mut thread_parameters_handle: FFIHandle = null_mut();

        let thread_parameters_create_result = thread_parameters_create(&mut thread_parameters_handle as *mut FFIHandle);
        assert!(!thread_parameters_handle.is_null());
        assert_eq!(thread_parameters_create_result, FFICode::Success);

        // Clean-up.
        // NOTE: `thread_parameters_destroy` positive path is already tested here.
        let thread_parameters_destroy_result = thread_parameters_destroy(thread_parameters_handle);
        assert_eq!(thread_parameters_destroy_result, FFICode::Success);
    }

    #[test]
    fn thread_parameters_destroy_null_handle() {
        let thread_parameters_destroy_result = thread_parameters_destroy(null_mut());
        assert_eq!(thread_parameters_destroy_result, FFICode::NullParameter);
    }

    #[test]
    #[cfg_attr(miri, ignore)]
    fn thread_parameters_scheduler_parameters_succeeds() {
        let mut thread_parameters_handle: FFIHandle = null_mut();

        let _ = thread_parameters_create(&mut thread_parameters_handle as *mut FFIHandle);

        let thread_parameters_scheduler_parameters_result =
            thread_parameters_scheduler_parameters(thread_parameters_handle, SchedulerPolicy::Fifo, 50);
        assert_eq!(thread_parameters_scheduler_parameters_result, FFICode::Success);

        // Clean-up.
        thread_parameters_destroy(thread_parameters_handle);
    }

    #[test]
    fn thread_parameters_scheduler_parameters_null_handle() {
        let thread_parameters_scheduler_parameters_result =
            thread_parameters_scheduler_parameters(null_mut(), SchedulerPolicy::Fifo, 50);
        assert_eq!(thread_parameters_scheduler_parameters_result, FFICode::NullParameter);
    }

    #[test]
    #[cfg_attr(miri, ignore)]
    fn thread_parameters_scheduler_parameters_invalid_priority() {
        let mut thread_parameters_handle: FFIHandle = null_mut();

        let _ = thread_parameters_create(&mut thread_parameters_handle as *mut FFIHandle);

        let thread_parameters_scheduler_parameters_result =
            thread_parameters_scheduler_parameters(thread_parameters_handle, SchedulerPolicy::Other, 50);
        assert_eq!(thread_parameters_scheduler_parameters_result, FFICode::InvalidArgument);

        // Clean-up.
        thread_parameters_destroy(thread_parameters_handle);
    }

    #[test]
    fn thread_parameters_affinity_succeeds() {
        let mut thread_parameters_handle: FFIHandle = null_mut();

        let _ = thread_parameters_create(&mut thread_parameters_handle as *mut FFIHandle);

        let affinity = [1, 2, 3];
        let thread_parameters_affinity_result =
            thread_parameters_affinity(thread_parameters_handle, affinity.as_ptr(), affinity.len());
        assert_eq!(thread_parameters_affinity_result, FFICode::Success);

        // Clean-up.
        thread_parameters_destroy(thread_parameters_handle);
    }

    #[test]
    fn thread_parameters_affinity_null_handle() {
        let affinity = [1, 2, 3];
        let thread_parameters_affinity_result =
            thread_parameters_affinity(null_mut(), affinity.as_ptr(), affinity.len());
        assert_eq!(thread_parameters_affinity_result, FFICode::NullParameter);
    }

    #[test]
    fn thread_parameters_affinity_null_affinity_zero_elements() {
        let mut thread_parameters_handle: FFIHandle = null_mut();

        let _ = thread_parameters_create(&mut thread_parameters_handle as *mut FFIHandle);

        let thread_parameters_affinity_result = thread_parameters_affinity(thread_parameters_handle, null_mut(), 0);
        assert_eq!(thread_parameters_affinity_result, FFICode::Success);

        // Clean-up.
        thread_parameters_destroy(thread_parameters_handle);
    }

    #[test]
    fn thread_parameters_affinity_null_affinity_many_elements() {
        let mut thread_parameters_handle: FFIHandle = null_mut();

        let _ = thread_parameters_create(&mut thread_parameters_handle as *mut FFIHandle);

        let affinity = [1, 2, 3];
        let thread_parameters_affinity_result =
            thread_parameters_affinity(thread_parameters_handle, null_mut(), affinity.len());
        assert_eq!(thread_parameters_affinity_result, FFICode::NullParameter);

        // Clean-up.
        thread_parameters_destroy(thread_parameters_handle);
    }

    #[test]
    fn thread_parameters_stack_size_succeeds() {
        let mut thread_parameters_handle: FFIHandle = null_mut();

        let _ = thread_parameters_create(&mut thread_parameters_handle as *mut FFIHandle);

        let thread_parameters_stack_size_result = thread_parameters_stack_size(thread_parameters_handle, 1024 * 1024);
        assert_eq!(thread_parameters_stack_size_result, FFICode::Success);

        // Clean-up.
        thread_parameters_destroy(thread_parameters_handle);
    }

    #[test]
    fn thread_parameters_stack_size_null_handle() {
        let thread_parameters_stack_size_result = thread_parameters_stack_size(null_mut(), 1024 * 1024);
        assert_eq!(thread_parameters_stack_size_result, FFICode::NullParameter);
    }
}
