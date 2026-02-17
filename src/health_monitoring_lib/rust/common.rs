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
use std::time::Instant;

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

/// Range of accepted time.
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

/// Get offset between HMON and monitor starting time points as [`u32`].
pub(crate) fn hmon_time_offset(hmon_starting_point: Instant, monitor_starting_point: Instant) -> u32 {
    let result = hmon_starting_point.checked_duration_since(monitor_starting_point);
    let duration_since = result.expect("HMON starting point is earlier than monitor starting point");
    duration_to_u32(duration_since)
}

/// Get duration as [`u32`].
pub(crate) fn duration_to_u32(duration: Duration) -> u32 {
    let millis = duration.as_millis();
    u32::try_from(millis).expect("Monitor running for too long")
}

/// Heartbeat monitor error subgroup.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, crate::log::ScoreDebug)]
pub(crate) enum HeartbeatMonitorEvaluationError {
    /// Multiple heartbeats were observed in the same epoch.
    MultipleHeartbeats,
}

/// Errors that can occur during monitor evaluation.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, crate::log::ScoreDebug)]
pub(crate) enum MonitorEvaluationError {
    TooEarly,
    TooLate,
    HeartbeatSpecific(HeartbeatMonitorEvaluationError),
}

/// Trait for evaluating monitors and reporting errors to be used by HealthMonitor.
pub(crate) trait MonitorEvaluator {
    /// Run monitor evaluation.
    ///
    /// - `hmon_starting_point` - starting point of all monitors.
    /// - `on_error` - error handling, containing tag of failing object and error code.
    fn evaluate(&self, hmon_starting_point: Instant, on_error: &mut dyn FnMut(&IdentTag, MonitorEvaluationError));
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
    fn evaluate(&self, hmon_starting_point: Instant, on_error: &mut dyn FnMut(&IdentTag, MonitorEvaluationError)) {
        self.inner.evaluate(hmon_starting_point, on_error)
    }
}

pub(crate) mod ffi {
    use core::mem::ManuallyDrop;
    use core::ops::{Deref, DerefMut};

    pub(crate) type FFIHandle = *mut core::ffi::c_void;

    /// Error representation.
    /// Must be aligned with `score::hm::Error` with additional success value.
    #[repr(u8)]
    pub(crate) enum FFIError {
        Success = 0,
        NotFound,
        AlreadyExists,
        #[allow(dead_code)]
        InvalidArgument,
        #[allow(dead_code)]
        WrongState,
        Failed,
    }

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

#[cfg(test)]
mod tests {
    use crate::common::{duration_to_u32, hmon_time_offset};
    use core::time::Duration;
    use std::time::Instant;

    #[test]
    fn test_hmon_time_offset_valid() {
        let monitor_starting_point = Instant::now();
        let hmon_starting_point = Instant::now();
        let offset = hmon_time_offset(hmon_starting_point, monitor_starting_point);
        // Allow small offset.
        assert!(offset < 10);
    }

    #[test]
    #[should_panic(expected = "HMON starting point is earlier than monitor starting point")]
    fn test_hmon_time_offset_wrong_order() {
        let hmon_starting_point = Instant::now();
        let monitor_starting_point = Instant::now();
        let _offset = hmon_time_offset(hmon_starting_point, monitor_starting_point);
    }

    #[test]
    #[should_panic(expected = "Monitor running for too long")]
    fn test_hmon_time_offset_diff_too_large() {
        const HUNDRED_DAYS_AS_SECS: u64 = 100 * 24 * 60 * 60;
        let monitor_starting_point = Instant::now();
        let hmon_starting_point = Instant::now()
            .checked_add(Duration::from_secs(HUNDRED_DAYS_AS_SECS))
            .unwrap();
        let _offset = hmon_time_offset(hmon_starting_point, monitor_starting_point);
    }

    #[test]
    fn test_duration_to_u32_valid() {
        let result = duration_to_u32(Duration::from_millis(1234));
        assert_eq!(result, 1234);
    }

    #[test]
    #[should_panic(expected = "Monitor running for too long")]
    fn test_duration_to_u32_too_large() {
        const HUNDRED_DAYS_AS_SECS: u64 = 100 * 24 * 60 * 60;
        let _result = duration_to_u32(Duration::from_secs(HUNDRED_DAYS_AS_SECS));
    }
}
