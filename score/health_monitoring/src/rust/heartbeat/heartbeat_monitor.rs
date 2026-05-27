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

use crate::common::{
    duration_to_int, time_offset, Monitor, MonitorEvalHandle, MonitorEvaluationError, MonitorEvaluator, TimeRange,
};
use crate::health_monitor::HealthMonitorError;
use crate::heartbeat::heartbeat_state::HeartbeatState;
use crate::log::{error, warn};
use crate::protected_memory::ProtectedMemoryAllocator;
use crate::tag::MonitorTag;
use core::sync::atomic::{AtomicU64, Ordering};
use core::time::Duration;
use score_log::ScoreDebug;
use std::sync::Arc;
use std::time::Instant;

/// Heartbeat evaluation errors.
#[derive(Debug, PartialEq, Eq, Clone, Copy, Hash, ScoreDebug)]
pub(crate) enum HeartbeatEvaluationError {
    /// Finished too early.
    TooEarly,
    /// Finished too late.
    TooLate,
    /// Multiple heartbeats observed.
    MultipleHeartbeats,
}

/// Builder for [`HeartbeatMonitor`].
#[derive(Debug)]
pub struct HeartbeatMonitorBuilder {
    /// Time range between heartbeats.
    range: TimeRange,
}

impl HeartbeatMonitorBuilder {
    /// Create a new [`HeartbeatMonitorBuilder`].
    ///
    /// - `range` - time range between heartbeats.
    pub fn new(range: TimeRange) -> Self {
        Self { range }
    }

    /// Build the [`HeartbeatMonitor`].
    ///
    /// - `monitor_tag` - tag of this monitor.
    /// - `internal_processing_cycle` - health monitor processing cycle.
    /// - `_allocator` - protected memory allocator.
    pub(crate) fn build(
        self,
        monitor_tag: MonitorTag,
        internal_processing_cycle: Duration,
        _allocator: &ProtectedMemoryAllocator,
    ) -> Result<HeartbeatMonitor, HealthMonitorError> {
        // Check range is valid.
        let range_min_ms = self.range.min.as_millis() as u64;
        let internal_processing_cycle_ms = internal_processing_cycle.as_millis() as u64;
        if range_min_ms * 2 <= internal_processing_cycle_ms {
            error!(
                "Internal processing cycle duration ({} ms) must be shorter than two shortest allowed ranges ({} ms).",
                internal_processing_cycle_ms, range_min_ms
            );
            return Err(HealthMonitorError::InvalidArgument);
        }

        let inner = Arc::new(HeartbeatMonitorInner::new(monitor_tag, self.range));
        Ok(HeartbeatMonitor::new(inner))
    }
}

/// Heartbeat monitor.
pub struct HeartbeatMonitor {
    inner: Arc<HeartbeatMonitorInner>,
}

impl HeartbeatMonitor {
    /// Create a new [`HeartbeatMonitor`] instance.
    fn new(inner: Arc<HeartbeatMonitorInner>) -> Self {
        Self { inner }
    }

    /// Provide a heartbeat.
    pub fn heartbeat(&self) {
        self.inner.heartbeat()
    }
}

impl Monitor for HeartbeatMonitor {
    fn get_eval_handle(&self) -> crate::common::MonitorEvalHandle {
        // TODO: rethink design - currently two `Arc`s are needed.
        MonitorEvalHandle::new(Arc::new(HeartbeatMonitorHandle {
            inner: Arc::clone(&self.inner),
            start_timestamp: AtomicU64::new(0),
        }))
    }
}

struct HeartbeatMonitorHandle {
    inner: Arc<HeartbeatMonitorInner>,
    /// Current cycle start timestamp.
    ///
    /// `AtomicU64` is used to allow mutability inside `Arc`.
    /// Variable is only accessed by worker thread.
    start_timestamp: AtomicU64,
}

impl MonitorEvaluator for HeartbeatMonitorHandle {
    fn evaluate(&self, hmon_starting_point: Instant, on_error: &mut dyn FnMut(&MonitorTag, MonitorEvaluationError)) {
        let start_timestamp = self.start_timestamp.load(Ordering::Acquire);
        let evaluate_result = self.inner.evaluate(start_timestamp, hmon_starting_point, on_error);
        if let Some(new_start_timestamp) = evaluate_result {
            self.start_timestamp.store(new_start_timestamp, Ordering::Release);
        }
    }
}

/// Time range using [`u64`].
#[derive(ScoreDebug)]
struct InternalRange {
    min: u64,
    max: u64,
}

impl InternalRange {
    /// Create range using provided values.
    fn new(min: u64, max: u64) -> Self {
        assert!(min <= max, "provided min is greater than provided max");
        Self { min, max }
    }

    /// Create range with values offset by timestamp.
    fn offset(&self, timestamp: u64) -> Self {
        let min = self
            .min
            .checked_add(timestamp)
            .expect("offset min overflow in InternalRange");
        let max = self
            .max
            .checked_add(timestamp)
            .expect("offset max overflow in InternalRange");
        Self::new(min, max)
    }
}

impl From<TimeRange> for InternalRange {
    fn from(value: TimeRange) -> Self {
        let min = duration_to_int(value.min);
        let max = duration_to_int(value.max);
        Self::new(min, max)
    }
}

pub(crate) struct HeartbeatMonitorInner {
    /// Tag of this monitor.
    monitor_tag: MonitorTag,

    /// Time range between heartbeats.
    range: InternalRange,

    /// Monitor starting point.
    monitor_starting_point: Instant,

    /// Current heartbeat state.
    /// Contains data in relation to [`Self::monitor_starting_point`].
    heartbeat_state: HeartbeatState,
}

impl HeartbeatMonitorInner {
    fn new(monitor_tag: MonitorTag, range: TimeRange) -> Self {
        let monitor_starting_point = Instant::now();
        let heartbeat_state = HeartbeatState::new();
        Self {
            monitor_tag,
            range: InternalRange::from(range),
            monitor_starting_point,
            heartbeat_state,
        }
    }

    /// Provide a heartbeat.
    fn heartbeat(&self) {
        // Get current timestamp.
        let monitor_now = duration_to_int(self.monitor_starting_point.elapsed());

        // Set heartbeat timestamp and update counter.
        let _ = self.heartbeat_state.update(|mut current_state| {
            current_state.set_heartbeat_timestamp(monitor_now);
            current_state.increment_counter();
            Some(current_state)
        });
    }

    pub fn evaluate(
        &self,
        start_timestamp: u64,
        hmon_starting_point: Instant,
        on_error: &mut dyn FnMut(&MonitorTag, MonitorEvaluationError),
    ) -> Option<u64> {
        // Get current timestamp, with offset to HMON time.
        let offset = time_offset(hmon_starting_point, self.monitor_starting_point)
            .expect("HMON starting point is earlier than monitor starting point");
        let monitor_now = offset + duration_to_int::<u64>(hmon_starting_point.elapsed());

        // Load and reset current monitor state.
        let snapshot = self.heartbeat_state.reset();

        // Get and recalculate snapshot timestamps.
        // IMPORTANT: first heartbeat is obtained when HMON time is unknown.
        // It is necessary to:
        // - use offset as cycle starting point.
        // - get heartbeat snapshot in relation to zero point.
        let start_timestamp = if start_timestamp > 0 { start_timestamp } else { offset };
        let heartbeat_timestamp = snapshot.heartbeat_timestamp();

        // Get allowed time range as absolute values.
        let range = self.range.offset(start_timestamp);

        // Check current counter state.
        let counter = snapshot.counter();
        // Disallow multiple heartbeats in same heartbeat cycle.
        if counter > 1 {
            warn!("Multiple heartbeats detected");
            on_error(&self.monitor_tag, HeartbeatEvaluationError::MultipleHeartbeats.into());
            return None;
        }
        // Handle no heartbeats.
        else if counter == 0 {
            // No heartbeats after time range is an error.
            // Otherwise it's accepted, but function should not continue.
            if monitor_now > range.max {
                let offset = monitor_now - range.max;
                warn!("No heartbeat detected, observed after range: {}", offset);
                on_error(&self.monitor_tag, HeartbeatEvaluationError::TooLate.into());
            }
            // Either way - execution is stopped here.
            return None;
        }

        // Check current heartbeat state.
        // Heartbeat before allowed range.
        if heartbeat_timestamp < range.min {
            let offset = range.min - heartbeat_timestamp;
            warn!("Heartbeat occurred too early, offset to range: {}", offset);
            on_error(&self.monitor_tag, HeartbeatEvaluationError::TooEarly.into());
            None
        }
        // Heartbeat after allowed range.
        else if heartbeat_timestamp > range.max {
            let offset = heartbeat_timestamp - range.max;
            warn!("Heartbeat occurred too late, offset to range: {}", offset);
            on_error(&self.monitor_tag, HeartbeatEvaluationError::TooLate.into());
            None
        }
        // Heartbeat in allowed state.
        else {
            // Update heartbeat monitor state with a current heartbeat as a beginning of a new cycle.
            Some(heartbeat_timestamp)
        }
    }
}

#[cfg(test)]
mod test_common {
    use crate::common::TimeRange;
    use core::time::Duration;
    use std::thread::sleep;
    use std::time::Instant;

    pub(super) const TAG: &str = "heartbeat_monitor";

    pub(super) fn sleep_until(target: Duration, start: Instant) {
        let elapsed = start.elapsed();
        let diff = target.saturating_sub(elapsed);
        sleep(diff)
    }

    pub(super) fn range_from_ms(min: u64, max: u64) -> TimeRange {
        TimeRange::new(Duration::from_millis(min), Duration::from_millis(max))
    }
}

#[score_testing_macros::test_mod_with_log]
#[cfg(all(test, not(loom)))]
mod tests {
    use crate::common::{Monitor, MonitorEvaluationError, MonitorEvaluator, TimeRange};
    use crate::health_monitor::HealthMonitorError;
    use crate::heartbeat::heartbeat_monitor::test_common::{range_from_ms, sleep_until, TAG};
    use crate::heartbeat::{HeartbeatEvaluationError, HeartbeatMonitor, HeartbeatMonitorBuilder};
    use crate::protected_memory::ProtectedMemoryAllocator;
    use crate::tag::MonitorTag;
    use core::sync::atomic::{AtomicBool, Ordering};
    use core::time::Duration;
    use std::sync::Arc;
    use std::thread::{sleep, spawn};
    use std::time::Instant;

    #[test]
    fn heartbeat_monitor_builder_build_ok() {
        let range = TimeRange::new(Duration::from_millis(500), Duration::from_millis(1000));
        let monitor_tag = MonitorTag::from("heartbeat_monitor");
        let internal_processing_cycle = Duration::from_millis(100);
        let allocator = ProtectedMemoryAllocator {};
        let result = HeartbeatMonitorBuilder::new(range).build(monitor_tag, internal_processing_cycle, &allocator);
        assert!(result.is_ok());
    }

    #[test]
    fn heartbeat_monitor_builder_build_invalid_internal_processing_cycle() {
        let range = TimeRange::new(Duration::from_millis(500), Duration::from_millis(1000));
        let monitor_tag = MonitorTag::from("heartbeat_monitor");
        let internal_processing_cycle = Duration::from_millis(1000);
        let allocator = ProtectedMemoryAllocator {};
        let result = HeartbeatMonitorBuilder::new(range).build(monitor_tag, internal_processing_cycle, &allocator);
        assert!(result.is_err_and(|e| e == HealthMonitorError::InvalidArgument));
    }

    fn create_monitor_single_cycle(range: TimeRange) -> HeartbeatMonitor {
        let monitor_tag = MonitorTag::from(TAG);
        let internal_processing_cycle = Duration::from_millis(1);
        let allocator = ProtectedMemoryAllocator {};
        HeartbeatMonitorBuilder::new(range)
            .build(monitor_tag, internal_processing_cycle, &allocator)
            .unwrap()
    }

    #[test]
    fn heartbeat_monitor_no_beat_evaluate_early() {
        let range = range_from_ms(80, 120);
        let monitor = create_monitor_single_cycle(range);
        let hmon_starting_point = Instant::now();

        // No beat happened, no error is expected.
        monitor
            .get_eval_handle()
            .evaluate(hmon_starting_point, &mut |monitor_tag, error| {
                panic!("error happened, tag: {monitor_tag:?}, error: {error:?}")
            });
    }

    #[test]
    #[cfg_attr(miri, ignore)]
    fn heartbeat_monitor_no_beat_evaluate_in_range() {
        let range = range_from_ms(80, 120);
        let monitor = create_monitor_single_cycle(range);
        let hmon_starting_point = Instant::now();

        // Wait until middle of range.
        sleep_until(Duration::from_millis(100), hmon_starting_point);

        // No beat happened, no error is expected.
        monitor
            .get_eval_handle()
            .evaluate(hmon_starting_point, &mut |monitor_tag, error| {
                panic!("error happened, tag: {monitor_tag:?}, error: {error:?}")
            });
    }

    #[test]
    fn heartbeat_monitor_no_beat_evaluate_late() {
        let range = range_from_ms(80, 120);
        let monitor = create_monitor_single_cycle(range);
        let hmon_starting_point = Instant::now();

        // Wait until late.
        sleep_until(Duration::from_millis(150), hmon_starting_point);

        // No beat happened, too late error is expected.
        monitor
            .get_eval_handle()
            .evaluate(hmon_starting_point, &mut |monitor_tag, error| {
                assert_eq!(*monitor_tag, MonitorTag::from(TAG));
                assert_eq!(error, HeartbeatEvaluationError::TooLate.into());
            });
    }

    fn beat_eval_test(
        beat_time: Duration,
        eval_time: Duration,
        on_error: &mut dyn FnMut(&MonitorTag, MonitorEvaluationError),
    ) {
        let range = range_from_ms(80, 120);
        let monitor = create_monitor_single_cycle(range);
        let hmon_starting_point = Instant::now();

        // Wait and beat.
        sleep_until(beat_time, hmon_starting_point);
        monitor.heartbeat();

        // Wait and evaluate.
        sleep_until(eval_time, hmon_starting_point);
        monitor.get_eval_handle().evaluate(hmon_starting_point, on_error);
    }

    fn beat_early_test(eval_time: Duration) {
        beat_eval_test(Duration::from_millis(25), eval_time, &mut |monitor_tag, error| {
            assert_eq!(*monitor_tag, MonitorTag::from(TAG));
            assert_eq!(error, HeartbeatEvaluationError::TooEarly.into());
        });
    }

    #[test]
    fn heartbeat_monitor_beat_early_evaluate_early() {
        beat_early_test(Duration::from_millis(50));
    }

    #[test]
    fn heartbeat_monitor_beat_early_evaluate_in_range() {
        beat_early_test(Duration::from_millis(100));
    }

    #[test]
    fn heartbeat_monitor_beat_early_evaluate_late() {
        beat_early_test(Duration::from_millis(150));
    }

    fn beat_in_range_test(eval_time: Duration) {
        beat_eval_test(Duration::from_millis(90), eval_time, &mut |monitor_tag, error| {
            panic!("error happened, tag: {monitor_tag:?}, error: {error:?}")
        });
    }

    #[test]
    fn heartbeat_monitor_beat_in_range_evaluate_in_range() {
        beat_in_range_test(Duration::from_millis(100));
    }

    #[test]
    fn heartbeat_monitor_beat_in_range_evaluate_late() {
        beat_in_range_test(Duration::from_millis(150));
    }

    #[test]
    fn heartbeat_monitor_beat_late_evaluate_late() {
        beat_eval_test(
            Duration::from_millis(150),
            Duration::from_millis(200),
            &mut |monitor_tag, error| {
                assert_eq!(*monitor_tag, MonitorTag::from(TAG));
                assert_eq!(error, HeartbeatEvaluationError::TooLate.into());
            },
        )
    }

    fn multiple_beats_eval_test(beat_time: Duration, eval_time: Duration) {
        let range = range_from_ms(80, 120);
        let monitor = create_monitor_single_cycle(range);
        let hmon_starting_point = Instant::now();

        // Wait and beat.
        sleep_until(beat_time, hmon_starting_point);
        const NUM_BEATS: usize = 10;
        for _ in 0..NUM_BEATS {
            monitor.heartbeat();
        }

        // Wait and evaluate.
        sleep_until(eval_time, hmon_starting_point);
        monitor
            .get_eval_handle()
            .evaluate(hmon_starting_point, &mut |monitor_tag, error| {
                assert_eq!(*monitor_tag, MonitorTag::from(TAG));
                assert_eq!(error, HeartbeatEvaluationError::MultipleHeartbeats.into());
            });
    }

    #[test]
    fn heartbeat_monitor_multiple_beats_early_evaluate_early() {
        multiple_beats_eval_test(Duration::from_millis(25), Duration::from_millis(50))
    }

    #[test]
    fn heartbeat_monitor_multiple_beats_early_evaluate_in_range() {
        multiple_beats_eval_test(Duration::from_millis(25), Duration::from_millis(100))
    }

    #[test]
    fn heartbeat_monitor_multiple_beats_early_evaluate_late() {
        multiple_beats_eval_test(Duration::from_millis(25), Duration::from_millis(150))
    }

    #[test]
    fn heartbeat_monitor_multiple_beats_in_range_evaluate_in_range() {
        multiple_beats_eval_test(Duration::from_millis(90), Duration::from_millis(100))
    }

    #[test]
    fn heartbeat_monitor_multiple_beats_in_range_evaluate_late() {
        multiple_beats_eval_test(Duration::from_millis(90), Duration::from_millis(150))
    }

    #[test]
    fn heartbeat_monitor_multiple_beats_late_evaluate_late() {
        multiple_beats_eval_test(Duration::from_millis(150), Duration::from_millis(200))
    }

    fn create_monitor_multiple_cycles(cycle: Duration) -> Arc<HeartbeatMonitor> {
        let range = range_from_ms(80, 120);
        let monitor_tag = MonitorTag::from(TAG);
        let allocator = ProtectedMemoryAllocator {};
        let monitor = HeartbeatMonitorBuilder::new(range)
            .build(monitor_tag, cycle, &allocator)
            .unwrap();
        Arc::new(monitor)
    }

    #[test]
    #[cfg_attr(miri, ignore)]
    fn heartbeat_monitor_cycle_early() {
        let cycle = Duration::from_millis(20);
        let monitor = create_monitor_multiple_cycles(cycle);
        let hmon_starting_point = Instant::now();

        // Run heartbeat thread.
        let monitor_clone = monitor.clone();
        let heartbeat_finished = Arc::new(AtomicBool::new(false));
        let heartbeat_finished_clone = heartbeat_finished.clone();
        let heartbeat_thread = spawn(move || {
            const NUM_BEATS: u32 = 3;
            const BEAT_INTERVAL: Duration = Duration::from_millis(100);
            for i in 1..NUM_BEATS {
                sleep_until(i * BEAT_INTERVAL, hmon_starting_point);
                monitor_clone.heartbeat();
            }

            // Perform a last heartbeat in shorter interval.
            sleep_until(
                NUM_BEATS * BEAT_INTERVAL - Duration::from_millis(40),
                hmon_starting_point,
            );
            monitor_clone.heartbeat();

            heartbeat_finished_clone.store(true, Ordering::Release);
        });

        // Run evaluation thread.
        let eval_handle = monitor.get_eval_handle();
        while !heartbeat_finished.load(Ordering::Acquire) {
            sleep(cycle);
            // Too early error is expected.
            eval_handle.evaluate(hmon_starting_point, &mut |monitor_tag, error| {
                assert_eq!(*monitor_tag, MonitorTag::from(TAG));
                assert_eq!(error, HeartbeatEvaluationError::TooEarly.into());
            });
        }

        heartbeat_thread.join().unwrap();
    }

    #[test]
    #[cfg_attr(miri, ignore)]
    fn heartbeat_monitor_cycle_in_range() {
        let cycle = Duration::from_millis(20);
        let monitor = create_monitor_multiple_cycles(cycle);
        let hmon_starting_point = Instant::now();

        // Run heartbeat thread.
        let monitor_clone = monitor.clone();
        let heartbeat_finished = Arc::new(AtomicBool::new(false));
        let heartbeat_finished_clone = heartbeat_finished.clone();
        let heartbeat_thread = spawn(move || {
            const NUM_BEATS: u32 = 3;
            const BEAT_INTERVAL: Duration = Duration::from_millis(100);
            for i in 1..=NUM_BEATS {
                sleep_until(i * BEAT_INTERVAL, hmon_starting_point);
                monitor_clone.heartbeat();
            }
            heartbeat_finished_clone.store(true, Ordering::Release);
        });

        // Run evaluation thread.
        let eval_handle = monitor.get_eval_handle();
        while !heartbeat_finished.load(Ordering::Acquire) {
            sleep(cycle);
            // No error is expected.
            eval_handle.evaluate(hmon_starting_point, &mut |monitor_tag, error| {
                panic!("error happened, tag: {monitor_tag:?}, error: {error:?}")
            });
        }

        heartbeat_thread.join().unwrap();
    }

    #[test]
    #[cfg_attr(miri, ignore)]
    fn heartbeat_monitor_cycle_late() {
        let cycle = Duration::from_millis(20);
        let monitor = create_monitor_multiple_cycles(cycle);
        let hmon_starting_point = Instant::now();

        // Run heartbeat thread.
        let monitor_clone = monitor.clone();
        let heartbeat_finished = Arc::new(AtomicBool::new(false));
        let heartbeat_finished_clone = heartbeat_finished.clone();
        let heartbeat_thread = spawn(move || {
            const NUM_BEATS: u32 = 3;
            const BEAT_INTERVAL: Duration = Duration::from_millis(100);
            for i in 1..NUM_BEATS {
                sleep_until(i * BEAT_INTERVAL, hmon_starting_point);
                monitor_clone.heartbeat();
            }

            // Perform a last heartbeat in shorter interval.
            sleep_until(
                NUM_BEATS * BEAT_INTERVAL + Duration::from_millis(40),
                hmon_starting_point,
            );
            monitor_clone.heartbeat();

            heartbeat_finished_clone.store(true, Ordering::Release);
        });

        // Run evaluation thread.
        let eval_handle = monitor.get_eval_handle();
        while !heartbeat_finished.load(Ordering::Acquire) {
            sleep(cycle);
            // No heartbeat or too late error is expected.
            eval_handle.evaluate(hmon_starting_point, &mut |monitor_tag, error| {
                assert_eq!(*monitor_tag, MonitorTag::from(TAG));
                assert_eq!(error, HeartbeatEvaluationError::TooLate.into());
            });
        }

        heartbeat_thread.join().unwrap();
    }

    #[test]
    fn heartbeat_monitor_timestamp_offset() {
        let range = range_from_ms(80, 120);
        let monitor = create_monitor_single_cycle(range);

        // Move away monitor creation and HMON starting point.
        sleep(Duration::from_millis(300));
        let hmon_starting_point = Instant::now();

        // Wait and beat.
        sleep_until(Duration::from_millis(90), hmon_starting_point);
        monitor.heartbeat();

        // Wait and evaluate.
        sleep_until(Duration::from_millis(100), hmon_starting_point);
        monitor
            .get_eval_handle()
            .evaluate(hmon_starting_point, &mut |monitor_tag, error| {
                panic!("error happened, tag: {monitor_tag:?}, error: {error:?}")
            });
    }
}

#[cfg(all(test, loom))]
mod loom_tests {
    use crate::common::{Monitor, MonitorEvaluator, TimeRange};
    use crate::heartbeat::heartbeat_monitor::test_common::{range_from_ms, sleep_until, TAG};
    use crate::heartbeat::{HeartbeatEvaluationError, HeartbeatMonitor, HeartbeatMonitorBuilder};
    use crate::protected_memory::ProtectedMemoryAllocator;
    use crate::tag::MonitorTag;
    use core::time::Duration;
    use loom::thread::spawn;
    use std::sync::Arc;
    use std::time::Instant;

    fn create_monitor_single_cycle(range: TimeRange) -> Arc<HeartbeatMonitor> {
        let monitor_tag = MonitorTag::from(TAG);
        let internal_processing_cycle = Duration::from_millis(1);
        let allocator = ProtectedMemoryAllocator {};
        let monitor = HeartbeatMonitorBuilder::new(range)
            .build(monitor_tag, internal_processing_cycle, &allocator)
            .unwrap();
        Arc::new(monitor)
    }

    #[test]
    fn heartbeat_monitor_heartbeat_evaluate_too_early() {
        loom::model(|| {
            let range = range_from_ms(30, 70);
            let monitor = create_monitor_single_cycle(range);
            let hmon_starting_point = Instant::now();

            // Perform heartbeat in a separate thread.
            let monitor_clone = monitor.clone();
            let heartbeat_thread = spawn(move || monitor_clone.heartbeat());

            // Evaluate.
            monitor
                .get_eval_handle()
                .evaluate(hmon_starting_point, &mut |monitor_tag, error| {
                    assert_eq!(*monitor_tag, MonitorTag::from(TAG));
                    assert_eq!(error, HeartbeatEvaluationError::TooEarly.into());
                });

            heartbeat_thread.join().unwrap();
        });
    }

    #[test]
    fn heartbeat_monitor_heartbeat_evaluate_in_range() {
        loom::model(|| {
            let range = range_from_ms(30, 70);
            let monitor = create_monitor_single_cycle(range);
            let hmon_starting_point = Instant::now();

            // Wait until in range.
            sleep_until(Duration::from_millis(50), hmon_starting_point);

            // Perform heartbeat in a separate thread.
            let monitor_clone = monitor.clone();
            let heartbeat_thread = spawn(move || monitor_clone.heartbeat());

            // Evaluate.
            monitor
                .get_eval_handle()
                .evaluate(hmon_starting_point, &mut |monitor_tag, error| {
                    panic!("error happened, tag: {monitor_tag:?}, error: {error:?}");
                });

            heartbeat_thread.join().unwrap();
        });
    }

    #[test]
    fn heartbeat_monitor_heartbeat_evaluate_too_late() {
        loom::model(|| {
            let range = range_from_ms(30, 70);
            let monitor = create_monitor_single_cycle(range);
            let hmon_starting_point = Instant::now();

            // Wait until too late.
            sleep_until(Duration::from_millis(100), hmon_starting_point);

            // Perform heartbeat in a separate thread.
            let monitor_clone = monitor.clone();
            let heartbeat_thread = spawn(move || monitor_clone.heartbeat());

            // Evaluate.
            let mut error_detected = false;
            monitor
                .get_eval_handle()
                .evaluate(hmon_starting_point, &mut |monitor_tag, error| {
                    assert_eq!(*monitor_tag, MonitorTag::from(TAG));
                    assert_eq!(error, HeartbeatEvaluationError::TooLate.into());
                    error_detected = true;
                });

            heartbeat_thread.join().unwrap();
            assert!(error_detected);
        });
    }
}
