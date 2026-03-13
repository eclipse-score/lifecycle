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
pub mod heartbeat;
pub mod logic;

use crate::common::{Monitor, MonitorEvalHandle};
use crate::deadline::{DeadlineMonitor, DeadlineMonitorBuilder};
use crate::heartbeat::{HeartbeatMonitor, HeartbeatMonitorBuilder};
use crate::log::{error, ScoreDebug};
use crate::logic::{LogicMonitor, LogicMonitorBuilder};
pub use common::TimeRange;
use containers::fixed_capacity::FixedCapacityVec;
use core::time::Duration;
use std::collections::HashMap;
pub use tag::{DeadlineTag, MonitorTag};

/// Health monitor errors.
#[derive(PartialEq, Eq, Debug, ScoreDebug)]
pub enum HealthMonitorError {
    /// Requested entry not found.
    NotFound,
    /// Provided argument is invalid.
    InvalidArgument,
    /// Current state is invalid.
    WrongState,
}

/// Builder for the [`HealthMonitor`].
#[derive(Default)]
pub struct HealthMonitorBuilder {
    deadline_monitor_builders: HashMap<MonitorTag, DeadlineMonitorBuilder>,
    heartbeat_monitor_builders: HashMap<MonitorTag, HeartbeatMonitorBuilder>,
    logic_monitor_builders: HashMap<MonitorTag, LogicMonitorBuilder>,
    supervisor_api_cycle: Duration,
    internal_processing_cycle: Duration,
}

impl HealthMonitorBuilder {
    /// Create a new [`HealthMonitorBuilder`] instance.
    pub fn new() -> Self {
        Self {
            deadline_monitor_builders: HashMap::new(),
            heartbeat_monitor_builders: HashMap::new(),
            logic_monitor_builders: HashMap::new(),
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

    /// Add a [`HeartbeatMonitor`] for the given [`MonitorTag`].
    ///
    /// - `monitor_tag` - unique tag for the [`HeartbeatMonitor`].
    /// - `monitor_builder` - monitor builder to finalize.
    ///
    /// # Note
    ///
    /// If a heartbeat monitor with the same tag already exists, it will be overwritten.
    pub fn add_heartbeat_monitor(mut self, monitor_tag: MonitorTag, monitor_builder: HeartbeatMonitorBuilder) -> Self {
        self.add_heartbeat_monitor_internal(monitor_tag, monitor_builder);
        self
    }

    /// Add a [`LogicMonitor`] for the given [`MonitorTag`].
    ///
    /// - `monitor_tag` - unique tag for the [`LogicMonitor`].
    /// - `monitor_builder` - monitor builder to finalize.
    ///
    /// # Note
    ///
    /// If a logic monitor with the same tag already exists, it will be overwritten.
    pub fn add_logic_monitor(mut self, monitor_tag: MonitorTag, monitor_builder: LogicMonitorBuilder) -> Self {
        self.add_logic_monitor_internal(monitor_tag, monitor_builder);
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
    pub fn build(self) -> Result<HealthMonitor, HealthMonitorError> {
        // Check cycle values.
        // `supervisor_api_cycle` must be a multiple of `internal_processing_cycle`.
        let supervisor_api_cycle_ms = self.supervisor_api_cycle.as_millis() as u64;
        let internal_processing_cycle_ms = self.internal_processing_cycle.as_millis() as u64;
        if !supervisor_api_cycle_ms.is_multiple_of(internal_processing_cycle_ms) {
            error!(
                "Supervisor API cycle duration ({} ms) must be a multiple of internal processing cycle interval ({} ms).",
                supervisor_api_cycle_ms, internal_processing_cycle_ms
            );
            return Err(HealthMonitorError::InvalidArgument);
        }

        // Check number of monitors.
        let num_monitors = self.deadline_monitor_builders.len()
            + self.heartbeat_monitor_builders.len()
            + self.logic_monitor_builders.len();
        if num_monitors == 0 {
            error!("No monitors have been added. HealthMonitor cannot be created.");
            return Err(HealthMonitorError::WrongState);
        }

        // Create allocator.
        let allocator = protected_memory::ProtectedMemoryAllocator {};

        // Create deadline monitors.
        let mut deadline_monitors = HashMap::new();
        for (tag, builder) in self.deadline_monitor_builders {
            let monitor = builder.build(tag, &allocator);
            deadline_monitors.insert(tag, Some(MonitorState::Available(monitor)));
        }

        // Create heartbeat monitors.
        let mut heartbeat_monitors = HashMap::new();
        for (tag, builder) in self.heartbeat_monitor_builders {
            let monitor = builder.build(tag, self.internal_processing_cycle, &allocator)?;
            heartbeat_monitors.insert(tag, Some(MonitorState::Available(monitor)));
        }

        // Create logic monitors.
        let mut logic_monitors = HashMap::new();
        for (tag, builder) in self.logic_monitor_builders {
            let monitor = builder.build(tag, &allocator)?;
            logic_monitors.insert(tag, Some(MonitorState::Available(monitor)));
        }

        Ok(HealthMonitor {
            deadline_monitors,
            heartbeat_monitors,
            logic_monitors,
            worker: worker::UniqueThreadRunner::new(self.internal_processing_cycle),
            supervisor_api_cycle: self.supervisor_api_cycle,
        })
    }

    // Used by FFI and config parsing code which prefer not to move builder instance

    pub(crate) fn add_deadline_monitor_internal(
        &mut self,
        monitor_tag: MonitorTag,
        monitor_builder: DeadlineMonitorBuilder,
    ) {
        self.deadline_monitor_builders.insert(monitor_tag, monitor_builder);
    }

    pub(crate) fn add_heartbeat_monitor_internal(
        &mut self,
        monitor_tag: MonitorTag,
        monitor_builder: HeartbeatMonitorBuilder,
    ) {
        self.heartbeat_monitor_builders.insert(monitor_tag, monitor_builder);
    }

    pub(crate) fn add_logic_monitor_internal(&mut self, monitor_tag: MonitorTag, monitor_builder: LogicMonitorBuilder) {
        self.logic_monitor_builders.insert(monitor_tag, monitor_builder);
    }

    pub(crate) fn with_supervisor_api_cycle_internal(&mut self, cycle_duration: Duration) {
        self.supervisor_api_cycle = cycle_duration;
    }

    pub(crate) fn with_internal_processing_cycle_internal(&mut self, cycle_duration: Duration) {
        self.internal_processing_cycle = cycle_duration;
    }
}

/// Monitor ownership state in the [`HealthMonitor`].
enum MonitorState<M> {
    /// Monitor is available.
    Available(M),
    /// Monitor is already taken.
    Taken(MonitorEvalHandle),
}

/// Monitor container.
/// - Must be an option to ensure monitor can be taken out (not referenced).
/// - Must be an enum to ensure evaluation handle is still available for HMON after monitor is taken.
type MonitorContainer<M> = Option<MonitorState<M>>;

/// Health monitor.
pub struct HealthMonitor {
    deadline_monitors: HashMap<MonitorTag, MonitorContainer<DeadlineMonitor>>,
    heartbeat_monitors: HashMap<MonitorTag, MonitorContainer<HeartbeatMonitor>>,
    logic_monitors: HashMap<MonitorTag, MonitorContainer<LogicMonitor>>,
    worker: worker::UniqueThreadRunner,
    supervisor_api_cycle: Duration,
}

impl HealthMonitor {
    fn get_monitor<M: Monitor>(
        monitors: &mut HashMap<MonitorTag, MonitorContainer<M>>,
        monitor_tag: MonitorTag,
    ) -> Option<M> {
        let monitor_state = monitors.get_mut(&monitor_tag)?;

        match monitor_state.take() {
            Some(MonitorState::Available(monitor)) => {
                monitor_state.replace(MonitorState::Taken(monitor.get_eval_handle()));
                Some(monitor)
            },
            Some(MonitorState::Taken(handle)) => {
                // Taken handle is inserted back.
                monitor_state.replace(MonitorState::Taken(handle));
                None
            },
            None => None,
        }
    }

    /// Get and pass ownership of a [`DeadlineMonitor`] for the given [`MonitorTag`].
    ///
    /// - `monitor_tag` - unique tag for the [`DeadlineMonitor`].
    ///
    /// Returns [`Some`] containing [`DeadlineMonitor`] if found and not taken.
    /// Otherwise returns [`None`].
    pub fn get_deadline_monitor(&mut self, monitor_tag: MonitorTag) -> Option<DeadlineMonitor> {
        Self::get_monitor(&mut self.deadline_monitors, monitor_tag)
    }

    /// Get and pass ownership of a [`HeartbeatMonitor`] for the given [`MonitorTag`].
    ///
    /// - `monitor_tag` - unique tag for the [`HeartbeatMonitor`].
    ///
    /// Returns [`Some`] containing [`HeartbeatMonitor`] if found and not taken.
    /// Otherwise returns [`None`].
    pub fn get_heartbeat_monitor(&mut self, monitor_tag: MonitorTag) -> Option<HeartbeatMonitor> {
        Self::get_monitor(&mut self.heartbeat_monitors, monitor_tag)
    }

    /// Get and pass ownership of a [`LogicMonitor`] for the given [`MonitorTag`].
    ///
    /// - `monitor_tag` - unique tag for the [`LogicMonitor`].
    ///
    /// Returns [`Some`] containing [`LogicMonitor`] if found and not taken.
    /// Otherwise returns [`None`].
    pub fn get_logic_monitor(&mut self, monitor_tag: MonitorTag) -> Option<LogicMonitor> {
        Self::get_monitor(&mut self.logic_monitors, monitor_tag)
    }

    fn collect_given_monitors<M>(
        monitors_to_collect: &mut HashMap<MonitorTag, MonitorContainer<M>>,
        collected_monitors: &mut FixedCapacityVec<MonitorEvalHandle>,
    ) -> Result<(), HealthMonitorError> {
        for (tag, monitor) in monitors_to_collect.iter_mut() {
            match monitor.take() {
                Some(MonitorState::Taken(handle)) => {
                    if collected_monitors.push(handle).is_err() {
                        // Should not fail - capacity was preallocated.
                        error!("Failed to push monitor handle.");
                        return Err(HealthMonitorError::WrongState);
                    }
                },
                Some(MonitorState::Available(m)) => {
                    // Reinsert into collection.
                    monitor.replace(MonitorState::Available(m));
                    error!(
                        "All monitors must be taken before starting HealthMonitor but {:?} is not taken.",
                        tag
                    );
                    return Err(HealthMonitorError::WrongState);
                },
                None => {
                    error!(
                        "Invalid monitor ({:?}) state encountered while starting HealthMonitor.",
                        tag
                    );
                    return Err(HealthMonitorError::WrongState);
                },
            }
        }
        Ok(())
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
    /// Health monitoring logic stops when the [`HealthMonitor`] is dropped.
    pub fn start(&mut self) -> Result<(), HealthMonitorError> {
        // Collect all monitors.
        let num_monitors = self.deadline_monitors.len() + self.heartbeat_monitors.len() + self.logic_monitors.len();
        let mut collected_monitors = FixedCapacityVec::new(num_monitors);
        Self::collect_given_monitors(&mut self.deadline_monitors, &mut collected_monitors)?;
        Self::collect_given_monitors(&mut self.heartbeat_monitors, &mut collected_monitors)?;
        Self::collect_given_monitors(&mut self.logic_monitors, &mut collected_monitors)?;

        // Start monitoring logic.
        let monitoring_logic = worker::MonitoringLogic::new(
            collected_monitors,
            self.supervisor_api_cycle,
            #[cfg(not(any(test, feature = "stub_supervisor_api_client")))]
            supervisor_api_client::score_supervisor_api_client::ScoreSupervisorAPIClient::new(),
            #[cfg(any(test, feature = "stub_supervisor_api_client"))]
            supervisor_api_client::stub_supervisor_api_client::StubSupervisorAPIClient::new(),
        );

        self.worker.start(monitoring_logic);
        Ok(())
    }

    //TODO: Add possibility to run HM in the current thread - ie in main
}

#[score_testing_macros::test_mod_with_log]
#[cfg(all(test, not(loom)))]
mod tests {
    use crate::common::TimeRange;
    use crate::deadline::DeadlineMonitorBuilder;
    use crate::heartbeat::HeartbeatMonitorBuilder;
    use crate::logic::LogicMonitorBuilder;
    use crate::tag::{MonitorTag, StateTag};
    use crate::{HealthMonitorBuilder, HealthMonitorError};
    use core::time::Duration;

    fn def_heartbeat_monitor_builder() -> HeartbeatMonitorBuilder {
        let range = TimeRange::new(Duration::from_millis(100), Duration::from_millis(200));
        HeartbeatMonitorBuilder::new(range)
    }

    fn def_logic_monitor_builder() -> LogicMonitorBuilder {
        let state1 = StateTag::from("state1");
        let state2 = StateTag::from("state2");
        LogicMonitorBuilder::new(state1)
            .add_state(state1, &[state2])
            .add_state(state2, &[state1])
    }

    #[test]
    fn health_monitor_builder_new_succeeds() {
        let health_monitor_builder = HealthMonitorBuilder::new();
        assert!(health_monitor_builder.deadline_monitor_builders.is_empty());
        assert!(health_monitor_builder.heartbeat_monitor_builders.is_empty());
        assert!(health_monitor_builder.logic_monitor_builders.is_empty());
        assert_eq!(health_monitor_builder.supervisor_api_cycle, Duration::from_millis(500));
        assert_eq!(
            health_monitor_builder.internal_processing_cycle,
            Duration::from_millis(100)
        );
    }

    #[test]
    fn health_monitor_builder_build_succeeds() {
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
        let heartbeat_monitor_builder = def_heartbeat_monitor_builder();
        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let logic_monitor_builder = def_logic_monitor_builder();

        let result = HealthMonitorBuilder::new()
            .add_deadline_monitor(deadline_monitor_tag, deadline_monitor_builder)
            .add_heartbeat_monitor(heartbeat_monitor_tag, heartbeat_monitor_builder)
            .add_logic_monitor(logic_monitor_tag, logic_monitor_builder)
            .build();
        assert!(result.is_ok());
    }

    #[test]
    fn health_monitor_builder_build_invalid_cycles() {
        let result = HealthMonitorBuilder::new()
            .with_supervisor_api_cycle(Duration::from_millis(123))
            .with_internal_processing_cycle(Duration::from_millis(100))
            .build();
        assert!(result.is_err_and(|e| e == HealthMonitorError::InvalidArgument));
    }

    #[test]
    fn health_monitor_builder_build_no_monitors() {
        let result = HealthMonitorBuilder::new().build();
        assert!(result.is_err_and(|e| e == HealthMonitorError::WrongState));
    }

    #[test]
    fn health_monitor_get_deadline_monitor_available() {
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(deadline_monitor_tag, deadline_monitor_builder)
            .build()
            .unwrap();

        let result = health_monitor.get_deadline_monitor(deadline_monitor_tag);
        assert!(result.is_some());
    }

    #[test]
    fn health_monitor_get_deadline_monitor_taken() {
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(deadline_monitor_tag, deadline_monitor_builder)
            .build()
            .unwrap();

        let _ = health_monitor.get_deadline_monitor(deadline_monitor_tag);
        let result = health_monitor.get_deadline_monitor(deadline_monitor_tag);
        assert!(result.is_none());
    }

    #[test]
    fn health_monitor_get_deadline_monitor_unknown() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(MonitorTag::from("deadline_monitor"), deadline_monitor_builder)
            .build()
            .unwrap();

        let result = health_monitor.get_deadline_monitor(MonitorTag::from("undefined_monitor"));
        assert!(result.is_none());
    }

    #[test]
    fn health_monitor_get_deadline_monitor_invalid_state() {
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(deadline_monitor_tag, deadline_monitor_builder)
            .build()
            .unwrap();

        // Inject broken state - unreachable otherwise.
        health_monitor.deadline_monitors.insert(deadline_monitor_tag, None);

        let result = health_monitor.get_deadline_monitor(deadline_monitor_tag);
        assert!(result.is_none());
    }

    #[test]
    fn health_monitor_get_heartbeat_monitor_available() {
        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
        let heartbeat_monitor_builder = def_heartbeat_monitor_builder();
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_heartbeat_monitor(heartbeat_monitor_tag, heartbeat_monitor_builder)
            .build()
            .unwrap();

        let result = health_monitor.get_heartbeat_monitor(heartbeat_monitor_tag);
        assert!(result.is_some());
    }

    #[test]
    fn health_monitor_get_heartbeat_monitor_taken() {
        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
        let heartbeat_monitor_builder = def_heartbeat_monitor_builder();
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_heartbeat_monitor(heartbeat_monitor_tag, heartbeat_monitor_builder)
            .build()
            .unwrap();

        let _ = health_monitor.get_heartbeat_monitor(heartbeat_monitor_tag);
        let result = health_monitor.get_heartbeat_monitor(heartbeat_monitor_tag);
        assert!(result.is_none());
    }

    #[test]
    fn health_monitor_get_heartbeat_monitor_unknown() {
        let heartbeat_monitor_builder = def_heartbeat_monitor_builder();
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_heartbeat_monitor(MonitorTag::from("heartbeat_monitor"), heartbeat_monitor_builder)
            .build()
            .unwrap();

        let result = health_monitor.get_heartbeat_monitor(MonitorTag::from("undefined_monitor"));
        assert!(result.is_none());
    }

    #[test]
    fn health_monitor_get_heartbeat_monitor_invalid_state() {
        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
        let heartbeat_monitor_builder = def_heartbeat_monitor_builder();
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_heartbeat_monitor(heartbeat_monitor_tag, heartbeat_monitor_builder)
            .build()
            .unwrap();

        // Inject broken state - unreachable otherwise.
        health_monitor.heartbeat_monitors.insert(heartbeat_monitor_tag, None);

        let result = health_monitor.get_heartbeat_monitor(heartbeat_monitor_tag);
        assert!(result.is_none());
    }

    #[test]
    fn health_monitor_get_logic_monitor_available() {
        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let logic_monitor_builder = def_logic_monitor_builder();
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_logic_monitor(logic_monitor_tag, logic_monitor_builder)
            .build()
            .unwrap();

        let result = health_monitor.get_logic_monitor(logic_monitor_tag);
        assert!(result.is_some());
    }

    #[test]
    fn health_monitor_get_logic_monitor_taken() {
        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let logic_monitor_builder = def_logic_monitor_builder();
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_logic_monitor(logic_monitor_tag, logic_monitor_builder)
            .build()
            .unwrap();

        let _ = health_monitor.get_logic_monitor(logic_monitor_tag);
        let result = health_monitor.get_logic_monitor(logic_monitor_tag);
        assert!(result.is_none());
    }

    #[test]
    fn health_monitor_get_logic_monitor_unknown() {
        let logic_monitor_builder = def_logic_monitor_builder();
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_logic_monitor(MonitorTag::from("logic_monitor"), logic_monitor_builder)
            .build()
            .unwrap();

        let result = health_monitor.get_logic_monitor(MonitorTag::from("undefined_monitor"));
        assert!(result.is_none());
    }

    #[test]
    fn health_monitor_get_logic_monitor_invalid_state() {
        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let logic_monitor_builder = def_logic_monitor_builder();
        let mut health_monitor = HealthMonitorBuilder::new()
            .add_logic_monitor(logic_monitor_tag, logic_monitor_builder)
            .build()
            .unwrap();

        // Inject broken state - unreachable otherwise.
        health_monitor.logic_monitors.insert(logic_monitor_tag, None);

        let result = health_monitor.get_logic_monitor(logic_monitor_tag);
        assert!(result.is_none());
    }

    #[test]
    fn health_monitor_start_succeeds() {
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
        let heartbeat_monitor_builder = def_heartbeat_monitor_builder();
        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let logic_monitor_builder = def_logic_monitor_builder();

        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(deadline_monitor_tag, deadline_monitor_builder)
            .add_heartbeat_monitor(heartbeat_monitor_tag, heartbeat_monitor_builder)
            .add_logic_monitor(logic_monitor_tag, logic_monitor_builder)
            .build()
            .unwrap();

        let _deadline_monitor = health_monitor.get_deadline_monitor(deadline_monitor_tag).unwrap();
        let _heartbeat_monitor = health_monitor.get_heartbeat_monitor(heartbeat_monitor_tag).unwrap();
        let _logic_monitor = health_monitor.get_logic_monitor(logic_monitor_tag).unwrap();

        let result = health_monitor.start();
        assert!(result.is_ok());
    }

    #[test]
    fn health_monitor_start_monitors_not_taken() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let heartbeat_monitor_builder = def_heartbeat_monitor_builder();
        let logic_monitor_builder = def_logic_monitor_builder();

        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(MonitorTag::from("deadline_monitor"), deadline_monitor_builder)
            .add_heartbeat_monitor(MonitorTag::from("heartbeat_monitor"), heartbeat_monitor_builder)
            .add_logic_monitor(MonitorTag::from("logic_monitor"), logic_monitor_builder)
            .build()
            .unwrap();

        let result = health_monitor.start();
        assert!(result.is_err_and(|e| e == HealthMonitorError::WrongState));
    }

    #[test]
    fn health_monitor_start_not_taken_then_restart() {
        let deadline_monitor_tag = MonitorTag::from("deadline_monitor");
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let heartbeat_monitor_tag = MonitorTag::from("heartbeat_monitor");
        let heartbeat_monitor_builder = def_heartbeat_monitor_builder();
        let logic_monitor_tag = MonitorTag::from("logic_monitor");
        let logic_monitor_builder = def_logic_monitor_builder();

        let mut health_monitor = HealthMonitorBuilder::new()
            .add_deadline_monitor(deadline_monitor_tag, deadline_monitor_builder)
            .add_heartbeat_monitor(heartbeat_monitor_tag, heartbeat_monitor_builder)
            .add_logic_monitor(logic_monitor_tag, logic_monitor_builder)
            .build()
            .unwrap();

        // Start without taking any monitor.
        let start_result = health_monitor.start();
        assert!(start_result.is_err_and(|e| e == HealthMonitorError::WrongState));

        // Take monitors.
        let get_deadline_monitor_result = health_monitor.get_deadline_monitor(deadline_monitor_tag);
        assert!(get_deadline_monitor_result.is_some());
        let get_heartbeat_monitor_result = health_monitor.get_heartbeat_monitor(heartbeat_monitor_tag);
        assert!(get_heartbeat_monitor_result.is_some());
        let get_logic_monitor_result = health_monitor.get_logic_monitor(logic_monitor_tag);
        assert!(get_logic_monitor_result.is_some());

        // Try to start again, this time should be successful.
        let start_result = health_monitor.start();
        assert!(start_result.is_ok());
    }
}
