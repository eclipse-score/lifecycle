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

mod common;
mod ffi;
mod log;
mod protected_memory;
mod supervisor_api_client;
mod tag;
mod worker;

pub mod deadline;

use crate::common::MonitorEvalHandle;
pub use common::TimeRange;
use containers::fixed_capacity::FixedCapacityVec;
use core::time::Duration;
use std::collections::HashMap;
use std::sync::Arc;
pub use tag::{DeadlineTag, MonitorTag};

/// Builder for the [`HealthMonitor`].
#[derive(Default)]
pub struct HealthMonitorBuilder {
    deadline_monitor_builders: HashMap<MonitorTag, DeadlineMonitorBuilder>,
    supervisor_api_cycle: Duration,
    internal_processing_cycle: Duration,
}

impl HealthMonitorBuilder {
    /// Create a new [`HealthMonitorBuilder`] instance.
    pub fn new() -> Self {
        Self {
            deadline_monitor_builders: HashMap::new(),
            supervisor_api_cycle: Duration::from_millis(500),
            internal_processing_cycle: Duration::from_millis(100),
        }
    }

    /// Add a [`DeadlineMonitor`] for the given [`MonitorTag`].
    ///
    /// - `monitor_tag` - unique tag for the [`DeadlineMonitor`].
    /// - `monitor_builder` - monitor builder to finalize.
    ///
    /// # Note
    ///
    /// If a deadline monitor with the same tag already exists, it will be overwritten.
    pub fn add_deadline_monitor(mut self, monitor_tag: MonitorTag, monitor_builder: DeadlineMonitorBuilder) -> Self {
        self.add_deadline_monitor_internal(monitor_tag, monitor_builder);
        self
    }

    /// Set the interval between supervisor API notifications.
    /// This duration determines how often the health monitor notifies the supervisor about system liveness.
    ///
    /// - `cycle_duration` - interval between notifications.
    pub fn with_supervisor_api_cycle(mut self, cycle_duration: Duration) -> Self {
        self.with_supervisor_api_cycle_internal(cycle_duration);
        self
    }

    /// Set the internal interval between health monitor evaluations.
    ///
    /// - `cycle_duration` - interval between evaluations.
    pub fn with_internal_processing_cycle(mut self, cycle_duration: Duration) -> Self {
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

    pub(crate) fn add_deadline_monitor_internal(
        &mut self,
        monitor_tag: MonitorTag,
        monitor_builder: DeadlineMonitorBuilder,
    ) {
        self.deadline_monitor_builders.insert(monitor_tag, monitor_builder);
    }

    pub(crate) fn with_supervisor_api_cycle_internal(&mut self, cycle_duration: Duration) {
        self.supervisor_api_cycle = cycle_duration;
    }

    pub(crate) fn with_internal_processing_cycle_internal(&mut self, cycle_duration: Duration) {
        self.internal_processing_cycle = cycle_duration;
    }

    pub(crate) fn check_cycle_args_internal(&self) -> bool {
        self.supervisor_api_cycle
            .as_millis()
            .is_multiple_of(self.internal_processing_cycle.as_millis())
    }

    pub(crate) fn build_internal(self) -> HealthMonitor {
        let allocator = protected_memory::ProtectedMemoryAllocator {};

        // Create deadline monitors.
        let mut deadline_monitors = HashMap::new();
        for (tag, builder) in self.deadline_monitor_builders {
            deadline_monitors.insert(tag, (MonitorState::Available, builder.build(tag, &allocator)));
        }

        HealthMonitor {
            deadline_monitors,
            worker: worker::UniqueThreadRunner::new(self.internal_processing_cycle),
            supervisor_api_cycle: self.supervisor_api_cycle,
        }
    }
}

/// Monitor ownership state in the [`HealthMonitor`].
enum MonitorState {
    /// Monitor is available.
    Available,
    /// Monitor is already taken.
    Taken,
}

/// Health monitor.
pub struct HealthMonitor {
    deadline_monitors: HashMap<MonitorTag, (MonitorState, Arc<DeadlineMonitorInner>)>,
    worker: worker::UniqueThreadRunner,
    supervisor_api_cycle: Duration,
}

impl HealthMonitor {
    /// Get and pass ownership of a [`DeadlineMonitor`] for the given [`MonitorTag`].
    ///
    /// - `monitor_tag` - unique tag for the [`DeadlineMonitor`].
    ///
    /// Returns [`Some`] containing [`DeadlineMonitor`] if found and not taken.
    /// Otherwise returns [`None`].
    pub fn get_deadline_monitor(&mut self, monitor_tag: MonitorTag) -> Option<DeadlineMonitor> {
        let (state, inner) = self.deadline_monitors.get_mut(&monitor_tag)?;
        match state {
            MonitorState::Available => {
                *state = MonitorState::Taken;
                Some(DeadlineMonitor::new(inner.clone()))
            },
            MonitorState::Taken => None,
        }
    }

    /// Start the health monitoring logic in a separate thread.
    ///
    /// From this point, the health monitor will periodically check monitors and notify the supervisor about system liveness.
    ///
    /// # Notes
    ///
    /// This method shall be called before `Lifecycle.running()`.
    /// Otherwise the supervisor might consider the process not alive.
    ///
    /// Health monitoring logic stop when the [`HealthMonitor`] is dropped.
    ///
    /// # Panics
    ///
    /// Method panics if no monitors have been added.
    pub fn start(&mut self) {
        // Check number of monitors.
        assert!(
            self.check_monitors_exist_internal(),
            "No monitors have been added. HealthMonitor cannot start without any monitors."
        );

        let deadline_monitors = match self.collect_deadline_monitors_internal() {
            Ok(m) => m,
            Err(e) => panic!("{}", e),
        };

        self.start_internal(deadline_monitors);
    }

    pub(crate) fn check_monitors_exist_internal(&self) -> bool {
        !self.deadline_monitors.is_empty()
    }

    pub(crate) fn collect_deadline_monitors_internal(
        &self,
    ) -> Result<FixedCapacityVec<Arc<DeadlineMonitorInner>>, String> {
        let mut collected_monitors = FixedCapacityVec::new(self.deadline_monitors.len());
        for (tag, (state, inner)) in self.deadline_monitors.iter() {
            match state {
                MonitorState::Taken => {
                    if collected_monitors.push(inner.clone()).is_err() {
                        // Should not fail - capacity was preallocated.
                        return Err("Failed to push monitor handle".to_string());
                    }
                },
                MonitorState::Available => {
                    return Err(format!(
                        "All monitors must be taken before starting HealthMonitor but {:?} is not taken.",
                        tag
                    ));
                },
            };
        }
        Ok(collected_monitors)
    }

    pub(crate) fn start_internal(&mut self, monitors: FixedCapacityVec<MonitorEvalHandle>) {
        let monitoring_logic = worker::MonitoringLogic::new(
            monitors,
            self.supervisor_api_cycle,
            #[cfg(not(any(test, feature = "stub_supervisor_api_client")))]
            supervisor_api_client::score_supervisor_api_client::ScoreSupervisorAPIClient::new(),
            #[cfg(any(test, feature = "stub_supervisor_api_client"))]
            supervisor_api_client::stub_supervisor_api_client::StubSupervisorAPIClient::new(),
        );

        self.worker.start(monitoring_logic)
    }

    //TODO: Add possibility to run HM in the current thread - ie in main
}

#[score_testing_macros::test_mod_with_log]
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    #[should_panic(expected = "No monitors have been added. HealthMonitor cannot start without any monitors.")]
    fn hm_with_no_monitors_shall_panic_on_start() {
        let health_monitor_builder = super::HealthMonitorBuilder::new();
        health_monitor_builder.build().start();
    }

    #[test]
    #[should_panic(expected = "supervisor API cycle must be multiple of internal processing cycle")]
    fn hm_with_wrong_cycle_fails_to_build() {
        super::HealthMonitorBuilder::new()
            .with_supervisor_api_cycle(Duration::from_millis(50))
            .build();
    }

    #[test]
    fn hm_with_taken_monitors_starts() {
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(MonitorTag::from("test_monitor"), DeadlineMonitorBuilder::new())
            .build();

        let _monitor = health_monitor.get_deadline_monitor(MonitorTag::from("test_monitor"));
        health_monitor.start();
    }

    #[test]
    #[should_panic(
        expected = "All monitors must be taken before starting HealthMonitor but MonitorTag(test_monitor) is not taken."
    )]
    fn hm_with_monitors_shall_not_start_with_not_taken_monitors() {
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(MonitorTag::from("test_monitor"), DeadlineMonitorBuilder::new())
            .build();

        health_monitor.start();
    }

    #[test]
    fn hm_get_deadline_monitor_works() {
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(MonitorTag::from("test_monitor"), DeadlineMonitorBuilder::new())
            .build();

        {
            let monitor = health_monitor.get_deadline_monitor(MonitorTag::from("test_monitor"));
            assert!(
                monitor.is_some(),
                "Expected to retrieve the deadline monitor, but got None"
            );
        }

        {
            let monitor = health_monitor.get_deadline_monitor(MonitorTag::from("test_monitor"));
            assert!(
                monitor.is_none(),
                "Expected None when retrieving the monitor a second time, but got Some"
            );
        }
    }
}
