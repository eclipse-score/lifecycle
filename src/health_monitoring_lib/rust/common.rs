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

use crate::deadline::DeadlineEvaluationError;
use crate::heartbeat::HeartbeatEvaluationError;
use crate::log::ScoreDebug;
use crate::tag::MonitorTag;
use core::hash::Hash;
use core::time::Duration;
use std::sync::Arc;
use std::time::Instant;

/// Range of accepted time.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct TimeRange {
    pub min: Duration,
    pub max: Duration,
}

impl TimeRange {
    /// Create [`TimeRange`] with specified range.
    /// Created range: `<min; max>`.
    ///
    /// # Panics
    ///
    /// `max` cannot be smaller than `min`.
    pub fn new(min: Duration, max: Duration) -> Self {
        Self::new_internal(min, max).expect("TimeRange min must be less than or equal to max")
    }

    /// Create [`TimeRange`] with specified interval and tolerance.
    /// Created range: `<interval - tolerance.min; interval + tolerance.max>`.
    ///
    /// # Panics
    ///
    /// `tolerance.min` cannot be smaller than `interval`.
    pub fn from_interval(interval: Duration, tolerance: TimeRange) -> Self {
        assert!(
            interval >= tolerance.min,
            "TimeRange interval must be greater than tolerance min"
        );
        let min = interval - tolerance.min;
        let max = interval + tolerance.max;
        Self { min, max }
    }

    /// Create new [`TimeRange`].
    /// [`None`] if `max` is smaller than `min`.
    pub(crate) fn new_internal(min: Duration, max: Duration) -> Option<Self> {
        if min <= max {
            Some(Self { min, max })
        } else {
            None
        }
    }
}

/// A monitor with an evaluation handle available.
pub(crate) trait Monitor {
    /// Get an evaluation handle for this monitor.
    ///
    /// # NOTE
    ///
    /// This evaluation handle is intended to be called from a background thread periodically.
    fn get_eval_handle(&self) -> MonitorEvalHandle;
}

/// Errors that can occur during monitor evaluation.
/// Contains failing monitor type.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, ScoreDebug)]
#[allow(dead_code)]
pub(crate) enum MonitorEvaluationError {
    Deadline(DeadlineEvaluationError),
    Heartbeat(HeartbeatEvaluationError),
    Logic,
}

impl From<DeadlineEvaluationError> for MonitorEvaluationError {
    fn from(value: DeadlineEvaluationError) -> Self {
        MonitorEvaluationError::Deadline(value)
    }
}

impl From<HeartbeatEvaluationError> for MonitorEvaluationError {
    fn from(value: HeartbeatEvaluationError) -> Self {
        MonitorEvaluationError::Heartbeat(value)
    }
}

/// Trait for evaluating monitors and reporting errors to be used by HealthMonitor.
pub(crate) trait MonitorEvaluator {
    /// Run monitor evaluation.
    ///
    /// - `hmon_starting_point` - starting point of all monitors.
    /// - `on_error` - error handling, containing tag of failing object and error code.
    fn evaluate(&self, hmon_starting_point: Instant, on_error: &mut dyn FnMut(&MonitorTag, MonitorEvaluationError));
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
    fn evaluate(&self, hmon_starting_point: Instant, on_error: &mut dyn FnMut(&MonitorTag, MonitorEvaluationError)) {
        self.inner.evaluate(hmon_starting_point, on_error)
    }
}

/// Get offset between two time points.
/// [`None`] is returned if `later_time_point` is actually earlier than `earlier_time_point`.
pub(crate) fn time_offset<T>(later_time_point: Instant, earlier_time_point: Instant) -> Option<T>
where
    T: TryFrom<u128>,
    <T as TryFrom<u128>>::Error: core::fmt::Debug,
{
    let duration_since = later_time_point.checked_duration_since(earlier_time_point)?;
    Some(duration_to_int(duration_since))
}

/// Get duration as an integer containing milliseconds.
pub(crate) fn duration_to_int<T>(duration: Duration) -> T
where
    T: TryFrom<u128>,
    <T as TryFrom<u128>>::Error: core::fmt::Debug,
{
    let millis = duration.as_millis();
    T::try_from(millis).expect("Duration is too big for the integer of this type")
}

#[cfg(all(test, not(loom)))]
mod tests {
    use crate::common::{duration_to_int, time_offset, TimeRange};
    use core::time::Duration;
    use std::time::Instant;

    #[test]
    fn time_range_new_valid() {
        let min = Duration::from_millis(100);
        let max = Duration::from_millis(200);
        let range = TimeRange::new(min, max);
        assert_eq!(range.min, min);
        assert_eq!(range.max, max);
    }

    #[test]
    #[should_panic(expected = "TimeRange min must be less than or equal to max")]
    fn time_range_new_wrong_order() {
        let min = Duration::from_millis(200);
        let max = Duration::from_millis(100);
        let _ = TimeRange::new(min, max);
    }

    #[test]
    fn time_range_from_interval_valid() {
        let interval = Duration::from_millis(100);
        let tolerance = TimeRange::new(Duration::from_millis(20), Duration::from_millis(50));
        let range = TimeRange::from_interval(interval, tolerance);
        assert_eq!(range.min, interval - tolerance.min);
        assert_eq!(range.max, interval + tolerance.max);
    }

    #[test]
    #[should_panic(expected = "TimeRange interval must be greater than tolerance min")]
    fn time_range_from_interval_lower_too_large() {
        let interval = Duration::from_millis(100);
        let tolerance = TimeRange::new(Duration::from_millis(200), Duration::from_millis(300));
        let _ = TimeRange::from_interval(interval, tolerance);
    }

    #[test]
    fn time_offset_valid() {
        let monitor_starting_point = Instant::now();
        let hmon_starting_point = Instant::now();
        let result = time_offset::<u32>(hmon_starting_point, monitor_starting_point);
        // Allow small offset.
        assert!(result.is_some_and(|o| o < 10));
    }

    #[test]
    fn time_offset_wrong_order() {
        let hmon_starting_point = Instant::now();
        let monitor_starting_point = Instant::now();
        let result = time_offset::<u32>(hmon_starting_point, monitor_starting_point);
        assert!(result.is_none());
    }

    #[test]
    #[should_panic(expected = "Duration is too big for the integer of this type")]
    fn time_offset_diff_too_large() {
        const HUNDRED_DAYS_AS_SECS: u64 = 100 * 24 * 60 * 60;
        let monitor_starting_point = Instant::now();
        let hmon_starting_point = Instant::now()
            .checked_add(Duration::from_secs(HUNDRED_DAYS_AS_SECS))
            .unwrap();
        let _ = time_offset::<u32>(hmon_starting_point, monitor_starting_point);
    }

    #[test]
    fn duration_to_int_valid() {
        let result: u32 = duration_to_int(Duration::from_millis(1234));
        assert_eq!(result, 1234);
    }

    #[test]
    #[should_panic(expected = "Duration is too big for the integer of this type")]
    fn duration_to_int_too_large() {
        const HUNDRED_DAYS_AS_SECS: u64 = 100 * 24 * 60 * 60;
        let _result: u32 = duration_to_int(Duration::from_secs(HUNDRED_DAYS_AS_SECS));
    }
}
