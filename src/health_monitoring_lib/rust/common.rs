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

use core::fmt::Debug;
use core::hash::Hash;
use core::time::Duration;
use std::sync::Arc;
/// Unique identifier for deadlines.
#[derive(Clone, Copy, Eq)]
#[repr(C)]
pub struct IdentTag {
    data: *const u8,
    len: usize,
} // Internal representation as a leaked string slice for now. It can be also an str to u64 conversion. Since this is internal only, we can change it later if needed.

unsafe impl Send for IdentTag {}
unsafe impl Sync for IdentTag {}

impl IdentTag {
    /// Returns the string representation of the IdentTag.
    pub fn new(data: &'static str) -> Self {
        Self {
            data: data.as_ptr(),
            len: data.len(),
        }
    }
}

impl Debug for IdentTag {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let bytes = unsafe { core::slice::from_raw_parts(self.data, self.len) };
        let s = unsafe { core::str::from_utf8_unchecked(bytes) }; // Safety: The underlying data was created out of valid str
        write!(f, "IdentTag({})", s)
    }
}

// Safety below for `from_raw_parts` -> Data was constructed from valid str
impl Hash for IdentTag {
    fn hash<H: core::hash::Hasher>(&self, state: &mut H) {
        let bytes = unsafe { core::slice::from_raw_parts(self.data, self.len) };
        bytes.hash(state);
    }
}
impl PartialEq for IdentTag {
    fn eq(&self, other: &Self) -> bool {
        let self_bytes = unsafe { core::slice::from_raw_parts(self.data, self.len) };
        let other_bytes = unsafe { core::slice::from_raw_parts(other.data, other.len) };
        self_bytes == other_bytes
    }
}

impl crate::log::ScoreDebug for IdentTag {
    fn fmt(&self, f: crate::log::Writer, spec: &crate::log::FormatSpec) -> Result<(), crate::log::Error> {
        let bytes = unsafe { core::slice::from_raw_parts(self.data, self.len) };
        crate::log::DebugStruct::new(f, spec, "IdentTag")
            .field("data", unsafe { &core::str::from_utf8_unchecked(bytes) }) // Safety: The underlying data was created out of valid str
            .finish()
    }
}

impl From<String> for IdentTag {
    fn from(value: String) -> Self {
        let leaked = value.leak();

        Self {
            data: leaked.as_ptr(),
            len: leaked.len(),
        }
    }
}

impl From<&str> for IdentTag {
    fn from(value: &str) -> Self {
        let leaked = value.to_string().leak();

        Self {
            data: leaked.as_ptr(),
            len: leaked.len(),
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct TimeRange {
    pub min: Duration,
    pub max: Duration,
}

impl TimeRange {
    pub fn new(min: Duration, max: Duration) -> Self {
        assert!(min <= max, "TimeRange min must be less than or equal to max");
        Self { min, max }
    }
}

/// Errors that can occur during monitor evaluation.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, crate::log::ScoreDebug)]
pub(crate) enum MonitorEvaluationError {
    TooEarly,
    TooLate,
}

/// Trait for evaluating monitors and reporting errors to be used by HealthMonitor.
pub(crate) trait MonitorEvaluator {
    fn evaluate(&self, on_error: &mut dyn FnMut(&IdentTag, MonitorEvaluationError));
}

/// Handle to a monitor evaluator, allowing for dynamic dispatch.
pub(crate) struct MonitorEvalHandle {
    inner: Arc<dyn MonitorEvaluator + Send + Sync>,
}

impl MonitorEvalHandle {
    pub(crate) fn new<T: MonitorEvaluator + Send + Sync + 'static>(inner: Arc<T>) -> Self {
        Self { inner }
    }
}

impl MonitorEvaluator for MonitorEvalHandle {
    fn evaluate(&self, on_error: &mut dyn FnMut(&IdentTag, MonitorEvaluationError)) {
        self.inner.evaluate(on_error)
    }
}

pub(crate) mod ffi {
    use core::mem::ManuallyDrop;
    use core::ops::{Deref, DerefMut};

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
}
