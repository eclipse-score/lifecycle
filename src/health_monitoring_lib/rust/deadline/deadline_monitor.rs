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
#![allow(dead_code)]

use super::common::DeadlineTemplate;
use crate::common::{IdentTag, MonitorEvaluationError, TimeRange};
use crate::common::{MonitorEvalHandle, MonitorEvaluator};
use crate::{
    deadline::{
        common::StateIndex,
        deadline_state::{DeadlineState, DeadlineStateSnapshot},
    },
    protected_memory::ProtectedMemoryAllocator,
};
use core::hash::Hash;
use std::{collections::HashMap, sync::Arc, time::Instant};

use crate::log::*;

///
/// Errors that can occur when working with DeadlineMonitor
///
#[derive(Debug, PartialEq, Eq, Clone, Copy, Hash)]
pub enum DeadlineMonitorError {
    /// The requested deadline is already in use (someone keeps the Deadline instance)
    DeadlineInUse,

    /// The requested deadline was not registered during builder phase
    DeadlineNotFound,
}

/// Errors that can occur when working with Deadline instances
#[derive(Debug, PartialEq, ScoreDebug, Eq, Clone, Copy, Hash)]
pub enum DeadlineError {
    DeadlineAlreadyFailed,
}

/// Builder for DeadlineMonitor
#[derive(Debug, Default)]
pub struct DeadlineMonitorBuilder {
    deadlines: HashMap<IdentTag, TimeRange>,
}

impl DeadlineMonitorBuilder {
    /// Creates a new DeadlineMonitorBuilder
    pub fn new() -> Self {
        Self {
            deadlines: HashMap::new(),
        }
    }

    /// Adds a deadline with the given tag and duration range to the monitor.
    pub fn add_deadline(mut self, tag: &IdentTag, range: TimeRange) -> Self {
        self.add_deadline_internal(tag, range);
        self
    }

    /// Builds the DeadlineMonitor with the configured deadlines.
    pub(crate) fn build(self, _allocator: &ProtectedMemoryAllocator) -> DeadlineMonitor {
        DeadlineMonitor::new(self.deadlines)
    }

    // Used by FFI and config parsing code which prefer not to move builder instance

    pub(super) fn add_deadline_internal(&mut self, tag: &IdentTag, range: TimeRange) {
        self.deadlines.insert(*tag, range);
    }
}

pub struct DeadlineMonitor {
    inner: Arc<DeadlineMonitorInner>,
}

#[derive(Debug, PartialEq, Eq, Clone, Copy, Hash, ScoreDebug)]
pub(crate) enum DeadlineEvaluationError {
    TooEarly,
    TooLate,
}

impl DeadlineMonitor {
    fn new(deadlines: HashMap<IdentTag, TimeRange>) -> Self {
        let mut active_deadlines = vec![];

        let deadlines = deadlines
            .into_iter()
            .enumerate()
            .map(|(index, (tag, range))| {
                active_deadlines.push((tag, DeadlineState::new()));
                (tag, DeadlineTemplate::new(range, StateIndex::new(index)))
            })
            .collect();

        Self {
            #[allow(clippy::arc_with_non_send_sync)] // This will be fixed once we add background thread
            inner: Arc::new(DeadlineMonitorInner {
                deadlines,
                active_deadlines: active_deadlines.into(),
                start_time: Instant::now(),
            }),
        }
    }

    /// Acquires a deadline instance for the given tag.
    /// # Returns
    ///  - Ok(Deadline) - if the deadline was acquired successfully.
    ///  - Err(DeadlineMonitorError::DeadlineInUse) - if the deadline is already in use
    ///  - Err(DeadlineMonitorError::DeadlineNotFound) - if the deadline tag is not registered
    pub fn get_deadline(&self, tag: &IdentTag) -> Result<Deadline, DeadlineMonitorError> {
        if let Some(template) = self.inner.deadlines.get(tag) {
            match template.acquire_deadline() {
                Some(range) => Ok(Deadline {
                    range,
                    tag: *tag,
                    monitor: Arc::clone(&self.inner),
                    state_index: template.assigned_state_index,
                }),
                None => Err(DeadlineMonitorError::DeadlineInUse),
            }
        } else {
            Err(DeadlineMonitorError::DeadlineNotFound)
        }
    }

    pub(crate) fn get_eval_handle(&self) -> MonitorEvalHandle {
        MonitorEvalHandle::new(Arc::clone(&self.inner))
    }

    /// Evaluates all active deadlines and reports any missed deadlines or underruns.
    ///
    /// # NOTE
    /// This function is intended to be called from a background thread periodically.
    fn evaluate(&self, on_failed: impl FnMut(&IdentTag, DeadlineEvaluationError)) {
        self.inner.evaluate(on_failed);
    }
}

/// Represents a deadline that can be started and stopped.
pub struct Deadline {
    range: TimeRange,
    tag: IdentTag,
    state_index: StateIndex,
    monitor: Arc<DeadlineMonitorInner>,
}

/// A handle representing a started deadline. When dropped, it stops the deadline.
pub struct DeadlineHandle<'a>(&'a mut Deadline);

impl DeadlineHandle<'_> {
    /// Stops the deadline. This is equivalent to dropping the handle.
    pub fn stop(self) {
        drop(self);
    }
}

impl Drop for DeadlineHandle<'_> {
    fn drop(&mut self) {
        self.0.stop_internal();
    }
}

impl Deadline {
    ///
    /// Starts the deadline - it will be monitored by health monitoring system.
    ///
    /// # Returns
    ///  - Ok(DeadlineHandle) - if the deadline was started successfully.
    ///  - Err(DeadlineError::DeadlineAlreadyFailed) - if the deadline was already missed before
    ///
    pub fn start(&mut self) -> Result<DeadlineHandle<'_>, DeadlineError> {
        // Safety: We ensure that the caller upholds the safety contract for FFI usage by using &'a mut self lifetime in DeadlineHandle
        unsafe { self.start_internal().map(|_| DeadlineHandle(self)) }
    }

    /// Starts the deadline - it will be monitored by health monitoring system.
    /// This function is for FFI usage only!
    ///
    /// # Safety
    ///
    /// Caller must ensure that deadline is not used until it's stopped.
    /// After this call You shall assure there's only a single owner of the `Deadline` instance and it does not call start before stopping.
    pub(super) unsafe fn start_internal(&mut self) -> Result<(), DeadlineError> {
        let now = self.monitor.now();
        let max_time = now + self.range.max.as_millis() as u32;

        let mut is_broken = false;
        let _ = self.monitor.active_deadlines[*self.state_index]
            .1
            .update(|current| {
                if current.is_running() || current.is_underrun() {
                    is_broken = true;
                    return None; // Deadline is already missed, do nothing
                }

                let mut new = DeadlineStateSnapshot::default();
                new.set_timestamp_ms(max_time);
                new.set_running();
                Some(new)
            });

        if is_broken {
            warn!(
                "Trying to start deadline {:?} that already failed",
                self.tag
            );
            Err(DeadlineError::DeadlineAlreadyFailed)
        } else {
            Ok(())
        }
    }

    pub(super) fn stop_internal(&mut self) {
        let now = self.monitor.now();
        let max = self.range.max.as_millis() as u32;
        let min = self.range.min.as_millis() as u32;

        let mut possible_err = (None, 0);

        let _ = self.monitor.active_deadlines[*self.state_index]
            .1
            .update(|mut current| {
                debug_assert!(
                    current.is_running(),
                    "Deadline({:?}) is not running when trying to stop",
                    self.tag
                );

                let expected = current.timestamp_ms();
                if expected < now {
                    possible_err = (Some(DeadlineEvaluationError::TooLate), now - expected);
                    return None; // Deadline missed, let state as is for BG thread to report
                }

                let start_time = expected - max;
                let earliest_time = start_time + min;

                if now < earliest_time {
                    // Finished too early, leave it for reporting by BG thread

                    current.set_underrun();
                    possible_err = (Some(DeadlineEvaluationError::TooEarly), earliest_time - now);
                    return Some(current);
                }

                Some(DeadlineStateSnapshot::default()) // Reset to stopped state as all fine
            });

        match possible_err {
            (Some(DeadlineEvaluationError::TooEarly), val) => {
                error!("Deadline {:?} stopped too early by {} ms", self.tag, val);
            }
            (Some(DeadlineEvaluationError::TooLate), val) => {
                error!("Deadline {:?} stopped too late by {} ms", self.tag, val);
            }
            (None, _) => {}
        }
    }

    // Here we add internal to start in case of FFI usage
}

impl core::fmt::Debug for Deadline {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_struct("Deadline")
            .field("range", &self.range)
            .field("tag", &self.tag)
            .field("state_index", &self.state_index)
            .finish()
    }
}

impl Drop for Deadline {
    fn drop(&mut self) {
        self.monitor.release_deadline(&self.tag);
    }
}

struct DeadlineMonitorInner {
    // Start time of the monitor creation to calculate relative timestamps
    start_time: Instant,

    // Templates for deadlines registered in the monitor to create `Deadline` instances.
    deadlines: HashMap<IdentTag, DeadlineTemplate>,

    // This is shared state. Each deadline template has assigned index into this array.
    // Each deadline instance updates its state (under given index) and the deadline pointing to a state is Single-Producer
    // On the other side there is background thread evaluating all deadlines states - this is Single-Consumer for each given state.
    active_deadlines: Arc<[(IdentTag, DeadlineState)]>,
}

impl MonitorEvaluator for DeadlineMonitorInner {
    fn evaluate(&self, on_error: &mut dyn FnMut(&IdentTag, MonitorEvaluationError)) {
        self.evaluate(|tag, deadline_failure| match deadline_failure {
            DeadlineEvaluationError::TooEarly => on_error(tag, MonitorEvaluationError::TooEarly),
            DeadlineEvaluationError::TooLate => on_error(tag, MonitorEvaluationError::TooLate),
        });
    }
}

impl DeadlineMonitorInner {
    fn release_deadline(&self, tag: &IdentTag) {
        if let Some(template) = self.deadlines.get(tag) {
            template.release_deadline();
        } else {
            unreachable!("Releasing unknown deadline tag: {:?}", tag);
        }
    }

    fn now(&self) -> u32 {
        let duration = self.start_time.elapsed();
        // As u32 can hold up to ~49 days in milliseconds, this should be sufficient for our use case
        // We still have a room up to 60bits timestamp if needed in future
        u32::try_from(duration.as_millis()).expect("Monitor running for too long")
    }

    fn evaluate(&self, mut on_failed: impl FnMut(&IdentTag, DeadlineEvaluationError)) {
        for (tag, deadline) in self.active_deadlines.iter() {
            let snapshot = deadline.snapshot();
            if snapshot.is_underrun() {
                // Deadline finished too early, report
                warn!("Deadline finished too early!");

                // Here we would normally report the underrun to the monitoring system
                on_failed(tag, DeadlineEvaluationError::TooEarly);
            } else if snapshot.is_running() {
                debug_assert!(
                    snapshot.is_stopped(),
                    "Deadline snapshot cannot be both running and stopped"
                );

                let now = self.now();
                let expected = snapshot.timestamp_ms();
                if now > expected {
                    // Deadline missed, report
                    warn!("Deadline missed! Expected: {}, now: {}", expected, now);

                    // Here we would normally report the missed deadline to the monitoring system
                    on_failed(tag, DeadlineEvaluationError::TooLate);
                }
            }
        }
    }
}

#[score_testing_macros::test_mod_with_log]
#[cfg(test)]
mod tests {
    use super::*;

    fn create_monitor_with_deadlines() -> DeadlineMonitor {
        let allocator = ProtectedMemoryAllocator {};
        DeadlineMonitorBuilder::new()
            .add_deadline(
                &IdentTag::from("deadline_long"),
                TimeRange::new(
                    core::time::Duration::from_secs(1),
                    core::time::Duration::from_secs(50),
                ),
            )
            .add_deadline(
                &IdentTag::from("deadline_fast"),
                TimeRange::new(
                    core::time::Duration::from_millis(0),
                    core::time::Duration::from_millis(50),
                ),
            )
            .build(&allocator)
    }

    fn create_monitor_with_multiple_running_deadlines() -> DeadlineMonitor {
        let allocator = ProtectedMemoryAllocator {};
        DeadlineMonitorBuilder::new()
            .add_deadline(
                &IdentTag::from("slow"),
                TimeRange::new(
                    core::time::Duration::from_secs(0),
                    core::time::Duration::from_secs(50),
                ),
            )
            .add_deadline(
                &IdentTag::from("deadline_fast1"),
                TimeRange::new(
                    core::time::Duration::from_millis(0),
                    core::time::Duration::from_millis(50),
                ),
            )
            .add_deadline(
                &IdentTag::from("deadline_fast2"),
                TimeRange::new(
                    core::time::Duration::from_millis(0),
                    core::time::Duration::from_millis(34),
                ),
            )
            .add_deadline(
                &IdentTag::from("deadline_fast3"),
                TimeRange::new(
                    core::time::Duration::from_millis(0),
                    core::time::Duration::from_millis(10),
                ),
            )
            .build(&allocator)
    }

    #[test]
    fn get_deadline_unknown_tag() {
        let monitor = create_monitor_with_deadlines();
        let result = monitor.get_deadline(&IdentTag::from("unknown"));
        assert_eq!(result.err(), Some(DeadlineMonitorError::DeadlineNotFound));
    }

    #[test]
    fn start_stop_deadline_within_range_works() {
        let monitor = create_monitor_with_deadlines();
        let mut deadline = monitor
            .get_deadline(&IdentTag::from("deadline_long"))
            .unwrap();
        let handle = deadline.start().unwrap();

        std::thread::sleep(core::time::Duration::from_millis(1001)); // Sleep to simulate work within the deadline range

        drop(handle); // stop the deadline

        monitor.evaluate(|tag, deadline_failure| {
            panic!(
                "Deadline {:?} should not have failed or underrun({:?})",
                tag, deadline_failure
            );
        });
    }

    #[test]
    fn start_stop_deadline_outside_ranges_is_error_when_dropped_before_evaluate() {
        let monitor = create_monitor_with_deadlines();
        let mut deadline = monitor
            .get_deadline(&IdentTag::from("deadline_long"))
            .unwrap();
        let handle = deadline.start().unwrap();

        drop(handle); // stop the deadline

        monitor.evaluate(|tag, deadline_failure| {
            assert_eq!(
                deadline_failure,
                DeadlineEvaluationError::TooEarly,
                "Deadline {:?} should not have failed({:?})",
                tag,
                deadline_failure
            );
        });
    }
    #[test]
    fn deadline_outside_time_range_is_error_when_dropped_after_evaluate() {
        let monitor = create_monitor_with_deadlines();
        let mut deadline = monitor
            .get_deadline(&IdentTag::from("deadline_long"))
            .unwrap();
        let handle = deadline.start().unwrap();

        // So deadline stop happens after evaluate, still it should be reported as failed

        monitor.evaluate(|tag, deadline_failure| {
            assert_eq!(
                deadline_failure,
                DeadlineEvaluationError::TooEarly,
                "Deadline {:?} should not have failed({:?})",
                tag,
                deadline_failure
            );
        });

        drop(handle); // stop the deadline
    }

    #[test]
    fn deadline_failed_on_first_run_and_then_restarted_is_evaluated_as_error() {
        let monitor = create_monitor_with_deadlines();
        let mut deadline = monitor
            .get_deadline(&IdentTag::from("deadline_long"))
            .unwrap();
        let handle = deadline.start().unwrap();

        // So deadline failed, then we start it again so it shall be already expired and also evaluation shall work
        drop(handle); // stop the deadline
        drop(deadline); // drop the deadline to release it
        let mut deadline = monitor
            .get_deadline(&IdentTag::from("deadline_long"))
            .unwrap();
        let handle = deadline.start();
        assert_eq!(handle.err(), Some(DeadlineError::DeadlineAlreadyFailed));

        monitor.evaluate(|tag, deadline_failure| {
            assert_eq!(
                deadline_failure,
                DeadlineEvaluationError::TooEarly,
                "Deadline {:?} should not have failed ({:?})",
                tag,
                deadline_failure
            );
        });
    }

    #[test]
    fn start_stop_deadline_outside_ranges_is_evaluated_as_error() {
        let monitor = create_monitor_with_deadlines();
        let mut deadline = monitor
            .get_deadline(&IdentTag::from("deadline_fast"))
            .unwrap();
        let handle = deadline.start().unwrap();

        drop(handle); // stop the deadline

        monitor.evaluate(|tag, deadline_failure| {
            assert_eq!(
                deadline_failure,
                DeadlineEvaluationError::TooLate,
                "Deadline {:?} should not have failed({:?})",
                tag,
                deadline_failure
            );
        });
    }

    #[test]
    fn monitor_with_multiple_running_deadlines() {
        let monitor = create_monitor_with_multiple_running_deadlines();

        let mut deadline = monitor
            .get_deadline(&IdentTag::from("deadline_fast1"))
            .unwrap();
        let _handle1 = deadline.start().unwrap();

        let mut deadline = monitor
            .get_deadline(&IdentTag::from("deadline_fast2"))
            .unwrap();
        let _handle2 = deadline.start().unwrap();

        let mut deadline = monitor
            .get_deadline(&IdentTag::from("deadline_fast3"))
            .unwrap();
        let _handle3 = deadline.start().unwrap();

        std::thread::sleep(core::time::Duration::from_millis(51)); // Sleep to simulate work within the deadline range

        let mut cnt = 0;

        monitor.evaluate(|tag, deadline_failure| {
            cnt += 1;
            assert_eq!(
                deadline_failure,
                DeadlineEvaluationError::TooLate,
                "Deadline {:?} should not have failed({:?})",
                tag,
                deadline_failure
            );
        });

        assert_eq!(cnt, 3, "All three deadlines should have been evaluated");
    }
}
