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

use crate::common::{Monitor, MonitorEvalHandle, MonitorEvaluationError, MonitorEvaluator, PhantomUnsync};
use crate::health_monitor::HealthMonitorError;
use crate::log::{error, warn, ScoreDebug};
use crate::logic::logic_state::LogicState;
use crate::protected_memory::ProtectedMemoryAllocator;
use crate::tag::{MonitorTag, StateTag};
use core::hash::Hash;
use core::marker::PhantomData;
use std::collections::HashMap;
use std::sync::Arc;
use std::time::Instant;

/// Internal OK state representation.
pub(super) const OK_STATE: u8 = 0;

/// Logic evaluation errors.
#[repr(u8)]
#[derive(Debug, PartialEq, Eq, Clone, Copy, Hash, ScoreDebug)]
pub enum LogicEvaluationError {
    /// State is unknown or cannot be determined.
    InvalidState = OK_STATE + 1,
    /// Transition is invalid.
    InvalidTransition,
    /// Unknown error.
    UnmappedError,
}

impl From<LogicEvaluationError> for u8 {
    fn from(value: LogicEvaluationError) -> Self {
        value as u8
    }
}

impl TryFrom<u8> for LogicEvaluationError {
    type Error = ();

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        const INVALID_STATE: u8 = LogicEvaluationError::InvalidState as u8;
        const INVALID_TRANSITION: u8 = LogicEvaluationError::InvalidTransition as u8;
        const UNMAPPED_ERROR: u8 = LogicEvaluationError::UnmappedError as u8;
        match value {
            INVALID_STATE => Ok(LogicEvaluationError::InvalidState),
            INVALID_TRANSITION => Ok(LogicEvaluationError::InvalidTransition),
            UNMAPPED_ERROR => Ok(LogicEvaluationError::UnmappedError),
            _ => Err(()),
        }
    }
}

/// Node containing state data.
struct StateNode {
    tag: StateTag,
    allowed_targets: Vec<StateTag>,
}

/// Builder for [`LogicMonitor`].
#[derive(Debug)]
pub struct LogicMonitorBuilder {
    /// Starting state.
    initial_state: StateTag,

    /// State graph.
    /// Contains state as a key and allowed transition targets as value.
    state_graph: HashMap<StateTag, Vec<StateTag>>,
}

impl LogicMonitorBuilder {
    /// Create a new [`LogicMonitorBuilder`].
    ///
    /// - `initial_state` - starting point.
    pub fn new(initial_state: StateTag) -> Self {
        Self {
            initial_state,
            state_graph: HashMap::new(),
        }
    }

    /// Add state along with allowed transitions.
    /// If state already exists - it is overwritten.
    pub fn add_state(mut self, state: StateTag, allowed_targets: &[StateTag]) -> Self {
        self.add_state_internal(state, allowed_targets);
        self
    }

    /// Build the [`LogicMonitor`].
    ///
    /// - `monitor_tag` - tag of this monitor.
    /// - `_allocator` - protected memory allocator.
    pub(crate) fn build(
        self,
        monitor_tag: MonitorTag,
        _allocator: &ProtectedMemoryAllocator,
    ) -> Result<LogicMonitor, HealthMonitorError> {
        // Check number of states.
        if self.state_graph.is_empty() {
            error!("No states have been added. LogicMonitor cannot be created.");
            return Err(HealthMonitorError::WrongState);
        }

        // Check transitions are between defined states.
        for (state, allowed_targets) in self.state_graph.iter() {
            for allowed_target in allowed_targets.iter() {
                if !self.state_graph.contains_key(allowed_target) {
                    error!(
                        "Undefined target state. Origin: {:?}, target: {:?}",
                        state, allowed_target
                    );
                    return Err(HealthMonitorError::InvalidArgument);
                }
            }
        }

        // Convert builder-internal representation into monitor-internal representation.
        let mut state_graph_vec = Vec::new();
        for (state, allowed_targets) in self.state_graph.into_iter() {
            state_graph_vec.push(StateNode {
                tag: state,
                allowed_targets,
            });
        }

        // Check initial state is defined, determine initial state index.
        let mut initial_state_index_option = None;
        for (index, node) in state_graph_vec.iter().enumerate() {
            if node.tag == self.initial_state {
                initial_state_index_option = Some(index);
            }
        }

        let initial_state_index = match initial_state_index_option {
            Some(index) => index,
            None => {
                error!("Undefined requested initial state: {:?}", self.initial_state);
                return Err(HealthMonitorError::InvalidArgument);
            },
        };

        let inner = Arc::new(LogicMonitorInner::new(
            monitor_tag,
            initial_state_index,
            state_graph_vec,
        ));
        Ok(LogicMonitor::new(inner))
    }

    // FFI internals.

    pub(crate) fn add_state_internal(&mut self, state: StateTag, allowed_targets: &[StateTag]) {
        self.state_graph.insert(state, allowed_targets.to_vec());
    }
}

/// Logic monitor.
pub struct LogicMonitor {
    inner: Arc<LogicMonitorInner>,
    _unsync: PhantomUnsync,
}

impl LogicMonitor {
    /// Create a new [`LogicMonitor`] instance.
    fn new(inner: Arc<LogicMonitorInner>) -> Self {
        Self {
            inner,
            _unsync: PhantomData,
        }
    }

    /// Perform transition to a new state.
    /// On success, current state is returned.
    pub fn transition(&self, state: StateTag) -> Result<StateTag, LogicEvaluationError> {
        self.inner.transition(state)
    }

    /// Current monitor state.
    pub fn state(&self) -> Result<StateTag, LogicEvaluationError> {
        self.inner.state()
    }
}

impl Monitor for LogicMonitor {
    fn get_eval_handle(&self) -> MonitorEvalHandle {
        MonitorEvalHandle::new(Arc::clone(&self.inner))
    }
}

struct LogicMonitorInner {
    /// Tag of this monitor.
    monitor_tag: MonitorTag,

    /// Current logic state.
    logic_state: LogicState,

    /// State graph.
    /// Contains state and allowed targets.
    state_graph: Vec<StateNode>,
}

impl MonitorEvaluator for LogicMonitorInner {
    fn evaluate(&self, _hmon_starting_point: Instant, on_error: &mut dyn FnMut(&MonitorTag, MonitorEvaluationError)) {
        let snapshot = self.logic_state.snapshot();
        if let Err(error) = snapshot.monitor_status() {
            warn!("Logic monitor error observed: {:?}", error);
            on_error(&self.monitor_tag, error.into());
        }
    }
}

impl LogicMonitorInner {
    fn new(monitor_tag: MonitorTag, initial_state_index: usize, state_graph: Vec<StateNode>) -> Self {
        let logic_state = LogicState::new(initial_state_index);
        LogicMonitorInner {
            monitor_tag,
            logic_state,
            state_graph,
        }
    }

    fn find_node_by_index(&self, state_index: usize) -> Result<&StateNode, LogicEvaluationError> {
        match self.state_graph.get(state_index) {
            Some(node) => Ok(node),
            None => Err(LogicEvaluationError::InvalidState),
        }
    }

    fn find_index_by_tag(&self, state_tag: StateTag) -> Result<usize, LogicEvaluationError> {
        for (index, state_node) in self.state_graph.iter().enumerate() {
            if state_node.tag == state_tag {
                return Ok(index);
            }
        }

        Err(LogicEvaluationError::InvalidState)
    }

    fn transition(&self, target_state: StateTag) -> Result<StateTag, LogicEvaluationError> {
        // Load current monitor state.
        let mut snapshot = self.logic_state.snapshot();

        // Disallow operation in erroneous state.
        if snapshot.monitor_status().is_err() {
            warn!("Current logic monitor state cannot be determined");
            return Err(LogicEvaluationError::InvalidState);
        }

        // Get name and allowed targets of current state.
        let current_state_index = snapshot.current_state_index();
        let current_state_node = self.find_node_by_index(current_state_index)?;

        // Check transition to a target state is valid.
        if !current_state_node.allowed_targets.contains(&target_state) {
            // Move to `InvalidTransition` if requested target state is not known.
            warn!(
                "Requested state transition is invalid: {:?} -> {:?}",
                current_state_node.tag, target_state
            );

            let error = LogicEvaluationError::InvalidTransition;
            snapshot.set_monitor_status(error);
            let _ = self.logic_state.swap(snapshot);
            return Err(error);
        }

        // Find index of target state, then change current state.
        let target_state_index = self.find_index_by_tag(target_state)?;
        snapshot.set_current_state_index(target_state_index);
        let _ = self.logic_state.swap(snapshot);

        Ok(target_state)
    }

    fn state(&self) -> Result<StateTag, LogicEvaluationError> {
        // Load current monitor state.
        let snapshot = self.logic_state.snapshot();

        // Disallow operation in erroneous state.
        if snapshot.monitor_status().is_err() {
            warn!("Current logic monitor state cannot be determined");
            return Err(LogicEvaluationError::InvalidState);
        }

        // Find current state.
        self.find_node_by_index(snapshot.current_state_index())
            .map(|node| node.tag)
    }
}

#[score_testing_macros::test_mod_with_log]
#[cfg(all(test, not(loom)))]
mod tests {
    use crate::common::MonitorEvaluator;
    use crate::health_monitor::HealthMonitorError;
    use crate::logic::{LogicEvaluationError, LogicMonitorBuilder};
    use crate::protected_memory::ProtectedMemoryAllocator;
    use crate::tag::{MonitorTag, StateTag};
    use std::time::Instant;

    #[test]
    fn logic_monitor_builder_new_succeeds() {
        let initial_state = StateTag::from("initial");
        let builder = LogicMonitorBuilder::new(initial_state);
        assert_eq!(builder.initial_state, initial_state);
        assert!(builder.state_graph.is_empty())
    }

    #[test]
    fn logic_monitor_builder_build_succeeds() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let result = LogicMonitorBuilder::new(from_state)
            .add_state(from_state, &[to_state])
            .add_state(to_state, &[])
            .build(monitor_tag, &allocator);
        assert!(result.is_ok());
    }

    #[test]
    fn logic_monitor_builder_build_no_states() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let initial_state = StateTag::from("initial");
        let result = LogicMonitorBuilder::new(initial_state).build(monitor_tag, &allocator);
        assert!(result.is_err_and(|e| e == HealthMonitorError::WrongState));
    }

    #[test]
    fn logic_monitor_builder_build_undefined_target() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let result = LogicMonitorBuilder::new(from_state)
            .add_state(from_state, &[to_state])
            .build(monitor_tag, &allocator);
        assert!(result.is_err_and(|e| e == HealthMonitorError::InvalidArgument));
    }

    #[test]
    fn logic_monitor_builder_build_undefined_initial_state() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let initial_state = StateTag::from("initial");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let result = LogicMonitorBuilder::new(initial_state)
            .add_state(from_state, &[to_state])
            .add_state(to_state, &[])
            .build(monitor_tag, &allocator);
        assert!(result.is_err_and(|e| e == HealthMonitorError::InvalidArgument));
    }

    #[test]
    fn logic_monitor_transition_succeeds() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let monitor = LogicMonitorBuilder::new(from_state)
            .add_state(from_state, &[to_state])
            .add_state(to_state, &[])
            .build(monitor_tag, &allocator)
            .unwrap();

        let result = monitor.transition(to_state);
        assert!(result.is_ok_and(|s| s == to_state));
    }

    #[test]
    fn logic_monitor_transition_unknown_node() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let monitor = LogicMonitorBuilder::new(from_state)
            .add_state(from_state, &[to_state])
            .add_state(to_state, &[])
            .build(monitor_tag, &allocator)
            .unwrap();

        let result = monitor.transition(StateTag::from("unknown"));
        assert!(result.is_err_and(|e| e == LogicEvaluationError::InvalidTransition));
    }

    #[test]
    fn logic_monitor_transition_indeterminate_current_state() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let monitor = LogicMonitorBuilder::new(from_state)
            .add_state(from_state, &[to_state])
            .add_state(to_state, &[])
            .build(monitor_tag, &allocator)
            .unwrap();

        // Trying to transition into unknown state causes monitor to move into indeterminate state.
        let _ = monitor.transition(StateTag::from("unknown"));

        // Try to move to known state.
        let result = monitor.transition(to_state);
        assert!(result.is_err_and(|e| e == LogicEvaluationError::InvalidState));
    }

    #[test]
    fn logic_monitor_transition_invalid_transition() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let state1 = StateTag::from("state1");
        let state2: StateTag = StateTag::from("state2");
        let state3 = StateTag::from("state3");
        let monitor = LogicMonitorBuilder::new(state1)
            .add_state(state1, &[state2])
            .add_state(state2, &[state3])
            .add_state(state3, &[])
            .build(monitor_tag, &allocator)
            .unwrap();

        let result = monitor.transition(state3);
        assert!(result.is_err_and(|e| e == LogicEvaluationError::InvalidTransition));
    }

    #[test]
    fn logic_monitor_state_succeeds() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let state1 = StateTag::from("state1");
        let state2: StateTag = StateTag::from("state2");
        let state3 = StateTag::from("state3");
        let monitor = LogicMonitorBuilder::new(state1)
            .add_state(state1, &[state2])
            .add_state(state2, &[state3])
            .add_state(state3, &[])
            .build(monitor_tag, &allocator)
            .unwrap();

        // Check state, perform transition to the next one.
        let result = monitor.state();
        assert!(result.is_ok_and(|s| s == state1));

        let _ = monitor.transition(state2);
        let result = monitor.state();
        assert!(result.is_ok_and(|s| s == state2));

        let _ = monitor.transition(state3);
        let result = monitor.state();
        assert!(result.is_ok_and(|s| s == state3));
    }

    #[test]
    fn logic_monitor_state_indeterminate_current_state() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let monitor = LogicMonitorBuilder::new(from_state)
            .add_state(from_state, &[to_state])
            .add_state(to_state, &[])
            .build(monitor_tag, &allocator)
            .unwrap();

        // Trying to transition into unknown state causes monitor to move into indeterminate state.
        let _ = monitor.transition(StateTag::from("unknown"));

        // Try to check state.
        let result = monitor.state();
        assert!(result.is_err_and(|e| e == LogicEvaluationError::InvalidState));
    }

    #[test]
    fn logic_monitor_evaluate_succeeds() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let monitor = LogicMonitorBuilder::new(from_state)
            .add_state(from_state, &[to_state])
            .add_state(to_state, &[])
            .build(monitor_tag, &allocator)
            .unwrap();
        let hmon_starting_point = Instant::now();

        monitor.inner.evaluate(hmon_starting_point, &mut |monitor_tag, error| {
            panic!("error happened, tag: {monitor_tag:?}, error: {error:?}")
        });

        let _ = monitor.transition(to_state);

        monitor.inner.evaluate(hmon_starting_point, &mut |monitor_tag, error| {
            panic!("error happened, tag: {monitor_tag:?}, error: {error:?}")
        });
    }

    #[test]
    fn logic_monitor_evaluate_invalid_state() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let monitor = LogicMonitorBuilder::new(from_state)
            .add_state(from_state, &[to_state])
            .add_state(to_state, &[])
            .build(monitor_tag, &allocator)
            .unwrap();
        let hmon_starting_point = Instant::now();

        monitor.inner.evaluate(hmon_starting_point, &mut |monitor_tag, error| {
            panic!("error happened, tag: {monitor_tag:?}, error: {error:?}")
        });

        let _ = monitor.transition(StateTag::from("unknown"));

        let mut error_happened = false;
        monitor
            .inner
            .evaluate(hmon_starting_point, &mut |monitor_tag_internal, error| {
                error_happened = true;
                assert_eq!(*monitor_tag_internal, monitor_tag);
                assert_eq!(error, LogicEvaluationError::InvalidTransition.into())
            });
        assert!(error_happened);
    }

    #[test]
    fn logic_monitor_evaluate_invalid_transition() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let state1 = StateTag::from("state1");
        let state2: StateTag = StateTag::from("state2");
        let state3 = StateTag::from("state3");
        let monitor = LogicMonitorBuilder::new(state1)
            .add_state(state1, &[state2])
            .add_state(state2, &[state3])
            .add_state(state3, &[])
            .build(monitor_tag, &allocator)
            .unwrap();
        let hmon_starting_point = Instant::now();

        monitor.inner.evaluate(hmon_starting_point, &mut |monitor_tag, error| {
            panic!("error happened, tag: {monitor_tag:?}, error: {error:?}")
        });

        let _ = monitor.transition(state3);

        let mut error_happened = false;
        monitor
            .inner
            .evaluate(hmon_starting_point, &mut |monitor_tag_internal, error| {
                error_happened = true;
                assert_eq!(*monitor_tag_internal, monitor_tag);
                assert_eq!(error, LogicEvaluationError::InvalidTransition.into())
            });
        assert!(error_happened);
    }
}
