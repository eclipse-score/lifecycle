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
    duration_to_u32, hmon_time_offset, HeartbeatMonitorEvaluationError, IdentTag, MonitorEvalHandle,
    MonitorEvaluationError, MonitorEvaluator, TimeRange,
};
use crate::heartbeat::heartbeat_state::{HeartbeatState, HeartbeatStateSnapshot};
use crate::log::warn;
use crate::protected_memory::ProtectedMemoryAllocator;
use core::time::Duration;
use score_log::ScoreDebug;
use std::sync::Arc;
use std::time::Instant;

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
    /// - `tag` - tag of this monitor.
    /// - `internal_processing_cycle` - health monitor processing cycle.
    /// - `_allocator` - protected memory allocator.
    ///
    /// # Panics
    ///
    /// Internal processing cycle must be shorter than doubled minimum time range.
    pub(crate) fn build(
        self,
        tag: IdentTag,
        internal_processing_cycle: Duration,
        _allocator: &ProtectedMemoryAllocator,
    ) -> HeartbeatMonitor {
        assert!(self.range.min * 2 > internal_processing_cycle);
        HeartbeatMonitor::new(tag, self.range)
    }
}

/// Heartbeat monitor.
pub struct HeartbeatMonitor {
    inner: Arc<HeartbeatMonitorInner>,
}

impl HeartbeatMonitor {
    /// Create a new [`HeartbeatMonitor`].
    fn new(tag: IdentTag, range: TimeRange) -> Self {
        Self {
            inner: Arc::new(HeartbeatMonitorInner::new(tag, range)),
        }
    }

    /// Provide a heartbeat.
    pub fn heartbeat(&self) {
        self.inner.heartbeat()
    }

    /// Get eval handle.
    pub(crate) fn get_eval_handle(&self) -> MonitorEvalHandle {
        MonitorEvalHandle::new(Arc::clone(&self.inner))
    }
}

/// Time range using [`u32`].
#[derive(ScoreDebug)]
struct InternalRange {
    min: u32,
    max: u32,
}

impl InternalRange {
    /// Create range using provided values.
    fn new(min: u32, max: u32) -> Self {
        assert!(min <= max, "provided min is greater than provided max");
        Self { min, max }
    }

    /// Create range with values offset by timestamp.
    fn offset(&self, timestamp: u32) -> Self {
        Self::new(self.min + timestamp, self.max + timestamp)
    }
}

impl From<TimeRange> for InternalRange {
    fn from(value: TimeRange) -> Self {
        let min = duration_to_u32(value.min);
        let max = duration_to_u32(value.max);
        Self::new(min, max)
    }
}

struct HeartbeatMonitorInner {
    /// Tag of this monitor.
    tag: IdentTag,
    /// Time range between heartbeats.
    range: InternalRange,
    /// Monitor starting point.
    /// Offset is calculated during evaluation in relation to provided health monitor starting point.
    monitor_starting_point: Instant,
    /// Current heartbeat state.
    /// Contains data in relation to [`Self::monitor_starting_point`].
    heartbeat_state: HeartbeatState,
}

impl MonitorEvaluator for HeartbeatMonitorInner {
    fn evaluate(&self, hmon_starting_point: Instant, on_error: &mut dyn FnMut(&IdentTag, MonitorEvaluationError)) {
        // Get current timestamp, with offset to HMON time.
        let offset = hmon_time_offset(hmon_starting_point, self.monitor_starting_point);
        let now = offset + duration_to_u32(hmon_starting_point.elapsed());

        // Load current monitor state.
        let snapshot = self.heartbeat_state.snapshot();

        // Get and recalculate snapshot timestamps.
        // IMPORTANT: first heartbeat is obtained when HMON time is unknown.
        // It is necessary to:
        // - use offset as cycle starting point.
        // - get heartbeat snapshot in relation to zero point.
        let (start_timestamp, heartbeat_timestamp) = if snapshot.post_init() {
            let start_timestamp = snapshot.start_timestamp();
            let heartbeat_timestamp = start_timestamp + snapshot.heartbeat_timestamp_offset();
            (start_timestamp, heartbeat_timestamp)
        } else {
            let start_timestamp = offset;
            let heartbeat_timestamp = snapshot.heartbeat_timestamp_offset();
            (start_timestamp, heartbeat_timestamp)
        };

        // Get allowed time range as absolute values.
        let range = self.range.offset(start_timestamp);

        // Check current counter state.
        let counter = snapshot.counter();
        // Disallow multiple heartbeats in same heartbeat cycle.
        if counter > 1 {
            warn!("Multiple heartbeats detected");
            on_error(
                &self.tag,
                MonitorEvaluationError::HeartbeatSpecific(HeartbeatMonitorEvaluationError::MultipleHeartbeats),
            );
            return;
        }
        // Handle no heartbeats.
        else if counter == 0 {
            // Disallow no heartbeats when already out of time range.
            // Stop execution if still in range.
            if now > range.max {
                let offset = now - range.max;
                warn!("No heartbeat detected, observed after range: {}", offset);
                on_error(&self.tag, MonitorEvaluationError::TooLate);
            }
            // Either way - execution is stopped here.
            return;
        }

        // Check current heartbeat state.
        // Heartbeat before allowed range.
        if heartbeat_timestamp < range.min {
            let offset = range.min - heartbeat_timestamp;
            warn!("Heartbeat occurred too early, offset to range: {}", offset);
            on_error(&self.tag, MonitorEvaluationError::TooEarly);
        }
        // Heartbeat after allowed range.
        else if heartbeat_timestamp > range.max {
            let offset = heartbeat_timestamp - range.max;
            warn!("Heartbeat occurred too late, offset to range: {}", offset);
            on_error(&self.tag, MonitorEvaluationError::TooLate);
        }
        // Heartbeat in allowed state.
        else {
            // Update heartbeat monitor state with a current heartbeat as a beginning of a new cycle.
            let _ = self
                .heartbeat_state
                .update(|_| Some(HeartbeatStateSnapshot::new(heartbeat_timestamp)));
        }
    }
}

impl HeartbeatMonitorInner {
    fn new(tag: IdentTag, range: TimeRange) -> Self {
        let monitor_starting_point = Instant::now();
        let heartbeat_state_snapshot = HeartbeatStateSnapshot::default();
        let heartbeat_state = HeartbeatState::new(heartbeat_state_snapshot);
        Self {
            tag,
            range: InternalRange::from(range),
            monitor_starting_point,
            heartbeat_state,
        }
    }

    /// Provide a heartbeat.
    fn heartbeat(&self) {
        // Get current timestamp.
        let now = duration_to_u32(self.monitor_starting_point.elapsed());

        // Set heartbeat timestamp and update counter.
        let _ = self.heartbeat_state.update(|mut state| {
            let start_ts = state.start_timestamp();
            state.set_heartbeat_timestamp_offset(now - start_ts);
            state.increment_counter();
            Some(state)
        });
    }
}

#[cfg(test)]
mod test_common {
    use crate::TimeRange;
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
    use crate::common::{HeartbeatMonitorEvaluationError, MonitorEvaluationError, MonitorEvaluator};
    use crate::heartbeat::heartbeat_monitor::test_common::{range_from_ms, sleep_until, TAG};
    use crate::heartbeat::{HeartbeatMonitor, HeartbeatMonitorBuilder};
    use crate::protected_memory::ProtectedMemoryAllocator;
    use crate::{IdentTag, TimeRange};
    use core::sync::atomic::{AtomicBool, Ordering};
    use core::time::Duration;
    use std::sync::Arc;
    use std::thread::{sleep, spawn};
    use std::time::Instant;

    fn create_monitor_single_cycle(range: TimeRange) -> HeartbeatMonitor {
        let tag = IdentTag::from(TAG);
        let internal_processing_cycle = Duration::from_millis(1);
        let allocator = ProtectedMemoryAllocator {};
        HeartbeatMonitorBuilder::new(range).build(tag, internal_processing_cycle, &allocator)
    }

    #[test]
    fn test_no_beat_evaluate_early() {
        let range = range_from_ms(80, 120);
        let monitor = create_monitor_single_cycle(range);
        let hmon_starting_point = Instant::now();

        // No beat happened, no error is expected.
        monitor.inner.evaluate(hmon_starting_point, &mut |tag, error| {
            panic!("error happened, tag: {tag:?}, error: {error:?}")
        });
    }

    #[test]
    fn test_no_beat_evaluate_in_range() {
        let range = range_from_ms(80, 120);
        let monitor = create_monitor_single_cycle(range);
        let hmon_starting_point = Instant::now();

        // Wait until middle of range.
        sleep_until(Duration::from_millis(100), hmon_starting_point);

        // No beat happened, no error is expected.
        monitor.inner.evaluate(hmon_starting_point, &mut |tag, error| {
            panic!("error happened, tag: {tag:?}, error: {error:?}")
        });
    }
    #[test]
    fn test_no_beat_evaluate_late() {
        let range = range_from_ms(80, 120);
        let monitor = create_monitor_single_cycle(range);
        let hmon_starting_point = Instant::now();

        // Wait until late.
        sleep_until(Duration::from_millis(150), hmon_starting_point);

        // No beat happened, too late error is expected.
        monitor.inner.evaluate(hmon_starting_point, &mut |tag, error| {
            assert_eq!(*tag, IdentTag::from(TAG));
            assert_eq!(error, MonitorEvaluationError::TooLate);
        });
    }

    fn beat_eval_test(
        beat_time: Duration,
        eval_time: Duration,
        on_error: &mut dyn FnMut(&IdentTag, MonitorEvaluationError),
    ) {
        let range = range_from_ms(80, 120);
        let monitor = create_monitor_single_cycle(range);
        let hmon_starting_point = Instant::now();

        // Wait and beat.
        sleep_until(beat_time, hmon_starting_point);
        monitor.heartbeat();

        // Wait and evaluate.
        sleep_until(eval_time, hmon_starting_point);
        monitor.inner.evaluate(hmon_starting_point, on_error);
    }

    fn beat_early_test(eval_time: Duration) {
        beat_eval_test(
            Duration::from_millis(25),
            eval_time,
            &mut |tag: &IdentTag, error: MonitorEvaluationError| {
                assert_eq!(*tag, IdentTag::from(TAG));
                assert_eq!(error, MonitorEvaluationError::TooEarly);
            },
        );
    }

    #[test]
    fn test_beat_early_evaluate_early() {
        beat_early_test(Duration::from_millis(50));
    }

    #[test]
    fn test_beat_early_evaluate_in_range() {
        beat_early_test(Duration::from_millis(100));
    }

    #[test]
    fn test_beat_early_evaluate_late() {
        beat_early_test(Duration::from_millis(150));
    }

    fn beat_in_range_test(eval_time: Duration) {
        beat_eval_test(Duration::from_millis(90), eval_time, &mut |tag, error| {
            panic!("error happened, tag: {tag:?}, error: {error:?}")
        });
    }

    #[test]
    fn test_beat_in_range_evaluate_in_range() {
        beat_in_range_test(Duration::from_millis(100));
    }

    #[test]
    fn test_beat_in_range_evaluate_late() {
        beat_in_range_test(Duration::from_millis(150));
    }

    #[test]
    fn test_beat_late_evaluate_late() {
        beat_eval_test(
            Duration::from_millis(150),
            Duration::from_millis(200),
            &mut |tag: &IdentTag, error: MonitorEvaluationError| {
                assert_eq!(*tag, IdentTag::from(TAG));
                assert_eq!(error, MonitorEvaluationError::TooLate);
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
        monitor.inner.evaluate(
            hmon_starting_point,
            &mut |tag: &IdentTag, error: MonitorEvaluationError| {
                assert_eq!(*tag, IdentTag::from(TAG));
                assert_eq!(
                    error,
                    MonitorEvaluationError::HeartbeatSpecific(HeartbeatMonitorEvaluationError::MultipleHeartbeats)
                );
            },
        );
    }

    #[test]
    fn test_multiple_beats_early_evaluate_early() {
        multiple_beats_eval_test(Duration::from_millis(25), Duration::from_millis(50))
    }

    #[test]
    fn test_multiple_beats_early_evaluate_in_range() {
        multiple_beats_eval_test(Duration::from_millis(25), Duration::from_millis(100))
    }

    #[test]
    fn test_multiple_beats_early_evaluate_late() {
        multiple_beats_eval_test(Duration::from_millis(25), Duration::from_millis(150))
    }

    #[test]
    fn test_multiple_beats_in_range_evaluate_in_range() {
        multiple_beats_eval_test(Duration::from_millis(90), Duration::from_millis(100))
    }

    #[test]
    fn test_multiple_beats_in_range_evaluate_late() {
        multiple_beats_eval_test(Duration::from_millis(90), Duration::from_millis(150))
    }

    #[test]
    fn test_multiple_beats_late_evaluate_late() {
        multiple_beats_eval_test(Duration::from_millis(150), Duration::from_millis(200))
    }

    fn create_monitor_multiple_cycles(cycle: Duration) -> Arc<HeartbeatMonitor> {
        let range = range_from_ms(80, 120);
        let tag = IdentTag::from(TAG);
        let allocator = ProtectedMemoryAllocator {};
        Arc::new(HeartbeatMonitorBuilder::new(range).build(tag, cycle, &allocator))
    }

    #[test]
    fn test_cycle_early() {
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
        while !heartbeat_finished.load(Ordering::Acquire) {
            sleep(cycle);
            // Too early error is expected.
            monitor.inner.evaluate(hmon_starting_point, &mut |tag, error| {
                assert_eq!(*tag, IdentTag::from(TAG));
                assert_eq!(error, MonitorEvaluationError::TooEarly);
            });
        }

        heartbeat_thread.join().unwrap();
    }

    #[test]
    fn test_cycle_in_range() {
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
        while !heartbeat_finished.load(Ordering::Acquire) {
            sleep(cycle);
            // No error is expected.
            monitor.inner.evaluate(hmon_starting_point, &mut |tag, error| {
                panic!("error happened, tag: {tag:?}, error: {error:?}")
            });
        }

        heartbeat_thread.join().unwrap();
    }

    #[test]
    fn test_cycle_late() {
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
        while !heartbeat_finished.load(Ordering::Acquire) {
            sleep(cycle);
            // No heartbeat or too late error is expected.
            monitor.inner.evaluate(hmon_starting_point, &mut |tag, error| {
                assert_eq!(*tag, IdentTag::from(TAG));
                assert!(error == MonitorEvaluationError::TooLate);
            });
        }

        heartbeat_thread.join().unwrap();
    }

    #[test]
    fn test_timestamp_offset() {
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
        monitor.inner.evaluate(hmon_starting_point, &mut |tag, error| {
            panic!("error happened, tag: {tag:?}, error: {error:?}")
        });
    }
}

#[cfg(all(test, loom))]
mod loom_tests {
    use crate::common::{MonitorEvaluationError, MonitorEvaluator};
    use crate::heartbeat::heartbeat_monitor::test_common::{range_from_ms, sleep_until, TAG};
    use crate::heartbeat::{HeartbeatMonitor, HeartbeatMonitorBuilder};
    use crate::protected_memory::ProtectedMemoryAllocator;
    use crate::{IdentTag, TimeRange};
    use core::time::Duration;
    use loom::thread::spawn;
    use std::sync::Arc;
    use std::time::Instant;

    fn create_monitor_single_cycle(range: TimeRange) -> Arc<HeartbeatMonitor> {
        let tag = IdentTag::from(TAG);
        let internal_processing_cycle = Duration::from_millis(1);
        let allocator = ProtectedMemoryAllocator {};
        Arc::new(HeartbeatMonitorBuilder::new(range).build(tag, internal_processing_cycle, &allocator))
    }

    #[test]
    fn test_heartbeat_evaluate_too_early() {
        loom::model(|| {
            let range = range_from_ms(30, 70);
            let monitor = create_monitor_single_cycle(range);
            let hmon_starting_point = Instant::now();

            // Perform heartbeat in a separate thread.
            let monitor_clone = monitor.clone();
            let heartbeat_thread = spawn(move || monitor_clone.heartbeat());

            // Evaluate.
            monitor.inner.evaluate(hmon_starting_point, &mut |tag, error| {
                assert_eq!(*tag, IdentTag::from(TAG));
                assert_eq!(error, MonitorEvaluationError::TooEarly);
            });

            heartbeat_thread.join().unwrap();
        });
    }

    #[test]
    fn test_heartbeat_evaluate_in_range() {
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
            monitor.inner.evaluate(hmon_starting_point, &mut |tag, error| {
                panic!("error happened, tag: {tag:?}, error: {error:?}");
            });

            heartbeat_thread.join().unwrap();
        });
    }

    #[test]
    fn test_heartbeat_evaluate_too_late() {
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
            monitor.inner.evaluate(hmon_starting_point, &mut |tag, error| {
                assert_eq!(*tag, IdentTag::from(TAG));
                assert_eq!(error, MonitorEvaluationError::TooLate);
                error_detected = true;
            });

            heartbeat_thread.join().unwrap();
            assert!(error_detected);
        });
    }
}
