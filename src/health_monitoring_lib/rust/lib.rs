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

use std::collections::HashMap;

mod common;
mod ffi;
mod log;
mod protected_memory;
mod worker;

pub mod deadline;
use crate::common::MonitorEvalHandle;
pub use common::{IdentTag, TimeRange};
use containers::fixed_capacity::FixedCapacityVec;

#[derive(Default)]
pub struct HealthMonitorBuilder {
    deadlines: HashMap<IdentTag, deadline::DeadlineMonitorBuilder>,
    supervisor_api_cycle: core::time::Duration,
    internal_processing_cycle: core::time::Duration,
}

impl HealthMonitorBuilder {
    /// Create a new [`HealthMonitorBuilder`] instance.
    pub fn new() -> Self {
        Self {
            deadlines: HashMap::new(),
            supervisor_api_cycle: core::time::Duration::from_millis(500),
            internal_processing_cycle: core::time::Duration::from_millis(100),
        }
    }

    /// Adds a deadline monitor for a specific identifier tag.
    /// # Arguments
    /// * `tag` - The unique identifier for the deadline monitor.
    /// * `monitor` - The builder for the deadline monitor.
    /// # Note
    /// If a monitor with the same tag already exists, it will be overwritten.
    pub fn add_deadline_monitor(mut self, tag: &IdentTag, monitor: deadline::DeadlineMonitorBuilder) -> Self {
        self.add_deadline_monitor_internal(tag, monitor);
        self
    }

    /// Sets the cycle duration for supervisor API notifications.
    /// This duration determines how often the health monitor notifies the supervisor that the system is alive.
    pub fn with_supervisor_api_cycle(mut self, cycle_duration: core::time::Duration) -> Self {
        self.with_supervisor_api_cycle_internal(cycle_duration);
        self
    }

    /// Sets the internal processing cycle duration.
    /// This duration determines how often the health monitor checks deadlines.
    pub fn with_internal_processing_cycle(mut self, cycle_duration: core::time::Duration) -> Self {
        self.with_internal_processing_cycle_internal(cycle_duration);
        self
    }

    /// Build a new [`HealthMonitor`] instance based on provided parameters.
    pub fn build(self) -> HealthMonitor {
        assert!(
            self.check_cycle_args_internal(),
            "supervisor API cycle must be multiple of internal processing cycle"
        );

        self.build_internal()
    }

    // Used by FFI and config parsing code which prefer not to move builder instance

    pub(crate) fn add_deadline_monitor_internal(&mut self, tag: &IdentTag, monitor: deadline::DeadlineMonitorBuilder) {
        self.deadlines.insert(*tag, monitor);
    }

    pub(crate) fn with_supervisor_api_cycle_internal(&mut self, cycle_duration: core::time::Duration) {
        self.supervisor_api_cycle = cycle_duration;
    }

    pub(crate) fn with_internal_processing_cycle_internal(&mut self, cycle_duration: core::time::Duration) {
        self.internal_processing_cycle = cycle_duration;
    }

    pub(crate) fn check_cycle_args_internal(&self) -> bool {
        self.supervisor_api_cycle
            .as_millis()
            .is_multiple_of(self.internal_processing_cycle.as_millis())
    }

    pub(crate) fn build_internal(self) -> HealthMonitor {
        let allocator = protected_memory::ProtectedMemoryAllocator {};
        let mut monitors = HashMap::new();
        for (tag, builder) in self.deadlines {
            monitors.insert(tag, Some(DeadlineMonitorState::Available(builder.build(&allocator))));
        }
        HealthMonitor {
            deadline_monitors: monitors,
            worker: worker::UniqueThreadRunner::new(self.internal_processing_cycle),
            supervisor_api_cycle: self.supervisor_api_cycle,
        }
    }
}

enum DeadlineMonitorState {
    Available(deadline::DeadlineMonitor),
    Taken(common::MonitorEvalHandle),
}

pub struct HealthMonitor {
    deadline_monitors: HashMap<IdentTag, Option<DeadlineMonitorState>>,
    worker: worker::UniqueThreadRunner,
    supervisor_api_cycle: core::time::Duration,
}

impl HealthMonitor {
    /// Retrieves and removes (hand over to user) a deadline monitor associated with the given identifier tag.
    /// # Arguments
    /// * `tag` - The unique identifier for the deadline monitor.
    /// # Returns
    /// An Option containing the DeadlineMonitor if found, or None if
    ///     - no monitor exists for the given tag or was already obtained
    ///
    pub fn get_deadline_monitor(&mut self, tag: &IdentTag) -> Option<deadline::DeadlineMonitor> {
        let monitor = self.deadline_monitors.get_mut(tag)?;

        match monitor.take() {
            Some(DeadlineMonitorState::Available(deadline_monitor)) => {
                monitor.replace(DeadlineMonitorState::Taken(deadline_monitor.get_eval_handle()));

                Some(deadline_monitor)
            },
            Some(DeadlineMonitorState::Taken(v)) => {
                monitor.replace(DeadlineMonitorState::Taken(v)); // Insert back
                None
            },
            None => None,
        }
    }

    pub(crate) fn check_monitors_exist_internal(&self) -> bool {
        !self.deadline_monitors.is_empty()
    }

    pub(crate) fn collect_monitors_internal(&mut self) -> Result<FixedCapacityVec<MonitorEvalHandle>, String> {
        let mut monitors = FixedCapacityVec::new(self.deadline_monitors.len());
        for (tag, monitor) in self.deadline_monitors.iter_mut() {
            match monitor.take() {
                Some(DeadlineMonitorState::Taken(handle)) => {
                    if monitors.push(handle).is_err() {
                        // Should not fail since we preallocated enough capacity
                        return Err("Failed to push monitor handle".to_string());
                    }
                },
                Some(DeadlineMonitorState::Available(_)) => {
                    return Err(format!(
                        "All monitors must be taken before starting HealthMonitor but {:?} is not taken.",
                        tag
                    ));
                },
                None => {
                    return Err(format!(
                        "Invalid monitor ({:?}) state encountered while starting HealthMonitor.",
                        tag
                    ));
                },
            }
        }
        Ok(monitors)
    }

    pub(crate) fn start_internal(&mut self, monitors: FixedCapacityVec<MonitorEvalHandle>) {
        let monitoring_logic = worker::MonitoringLogic::new(
            monitors,
            self.supervisor_api_cycle,
            // Currently only `ScoreSupervisorAPIClient` and `StubSupervisorAPIClient` are supported.
            // The later is meant to be used for testing purposes.
            #[cfg(not(any(test, feature = "stub_supervisor_api_client")))]
            worker::ScoreSupervisorAPIClient::new(),
            #[cfg(any(test, feature = "stub_supervisor_api_client"))]
            worker::StubSupervisorAPIClient {},
        );

        self.worker.start(monitoring_logic)
    }

    /// Starts the health monitoring logic in a separate thread.
    ///
    /// From this point, the health monitor will periodically check monitors and notify the supervisor about system liveness.
    ///
    /// # Note
    ///  - This function shall be called before Lifecycle.running() otherwise the supervisor might consider the process not alive.
    ///  - Stops when the HealthMonitor instance is dropped.
    ///
    /// Panics if no monitors have been added.
    pub fn start(&mut self) {
        assert!(
            self.check_monitors_exist_internal(),
            "No deadline monitors have been added. HealthMonitor cannot start without any monitors."
        );

        let monitors = match self.collect_monitors_internal() {
            Ok(m) => m,
            Err(e) => panic!("{}", e),
        };

        self.start_internal(monitors);
    }

    //TODO: Add possibility to run HM in the current thread - ie in main
}

#[score_testing_macros::test_mod_with_log]
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    #[should_panic(expected = "No deadline monitors have been added. HealthMonitor cannot start without any monitors.")]
    fn hm_with_no_monitors_shall_panic_on_start() {
        let health_monitor_builder = super::HealthMonitorBuilder::new();
        health_monitor_builder.build().start();
    }

    #[test]
    #[should_panic(expected = "supervisor API cycle must be multiple of internal processing cycle")]
    fn hm_with_wrong_cycle_fails_to_build() {
        super::HealthMonitorBuilder::new()
            .with_supervisor_api_cycle(core::time::Duration::from_millis(50))
            .build();
    }

    #[test]
    fn hm_with_taken_monitors_starts() {
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(&IdentTag::new("test_monitor"), deadline::DeadlineMonitorBuilder::new())
            .build();

        let _monitor = health_monitor.get_deadline_monitor(&IdentTag::new("test_monitor"));
        health_monitor.start();
    }

    #[test]
    #[should_panic(
        expected = "All monitors must be taken before starting HealthMonitor but IdentTag(test_monitor) is not taken."
    )]
    fn hm_with_monitors_shall_not_start_with_not_taken_monitors() {
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(&IdentTag::new("test_monitor"), deadline::DeadlineMonitorBuilder::new())
            .build();

        health_monitor.start();
    }

    #[test]
    fn hm_get_deadline_monitor_works() {
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(&IdentTag::new("test_monitor"), deadline::DeadlineMonitorBuilder::new())
            .build();

        {
            let monitor = health_monitor.get_deadline_monitor(&IdentTag::new("test_monitor"));
            assert!(
                monitor.is_some(),
                "Expected to retrieve the deadline monitor, but got None"
            );
        }

        {
            let monitor = health_monitor.get_deadline_monitor(&IdentTag::new("test_monitor"));
            assert!(
                monitor.is_none(),
                "Expected None when retrieving the monitor a second time, but got Some"
            );
        }
    }
}
