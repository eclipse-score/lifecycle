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

use crate::common::{Monitor, MonitorEvalHandle, MonitorEvaluationError, MonitorEvaluator};
use crate::health_monitor::HealthMonitorError;
use crate::log::{error, warn, ScoreDebug};
use crate::protected_memory::ProtectedMemoryAllocator;
use crate::tag::{MonitorTag, StateTag};
use core::hash::{Hash, Hasher};
use core::sync::atomic::{AtomicU64, AtomicU8, Ordering};
use std::hash::DefaultHasher;
use std::sync::Arc;
use std::time::Instant;

/// Hashed representation of state.
#[derive(PartialEq, Eq)]
struct HashedState(u64);

impl From<u64> for HashedState {
    fn from(value: u64) -> Self {
        Self(value)
    }
}

impl From<HashedState> for u64 {
    fn from(value: HashedState) -> Self {
        value.0
    }
}

impl From<StateTag> for HashedState {
    fn from(value: StateTag) -> Self {
        Self::from(&value)
    }
}

impl From<&StateTag> for HashedState {
    fn from(value: &StateTag) -> Self {
        let mut hasher = DefaultHasher::new();
        value.hash(&mut hasher);
        Self(hasher.finish())
    }
}

/// Internal OK state representation.
const OK_STATE: u8 = 0;

/// Logic evaluation errors.
#[repr(u8)]
#[derive(Debug, PartialEq, Eq, Clone, Copy, Hash, ScoreDebug)]
pub enum LogicEvaluationError {
    /// State is unknown or cannot be determined.
    InvalidState = OK_STATE + 1,
    /// Transition is invalid.
    InvalidTransition,
}

impl From<LogicEvaluationError> for u8 {
    fn from(value: LogicEvaluationError) -> Self {
        value as u8
    }
}

impl From<u8> for LogicEvaluationError {
    fn from(value: u8) -> Self {
        match value {
            value if value == LogicEvaluationError::InvalidState as u8 => LogicEvaluationError::InvalidState,
            value if value == LogicEvaluationError::InvalidTransition as u8 => LogicEvaluationError::InvalidTransition,
            _ => panic!("Invalid underlying value of logic evaluation error."),
        }
    }
}

/// Builder for [`LogicMonitor`].
#[derive(Debug)]
pub struct LogicMonitorBuilder {
    /// Starting state.
    initial_state: StateTag,

    /// List of allowed states.
    allowed_states: Vec<StateTag>,

    /// List of allowed transitions between states.
    allowed_transitions: Vec<(StateTag, StateTag)>,
}

impl LogicMonitorBuilder {
    /// Create a new [`LogicMonitorBuilder`].
    ///
    /// - `initial_state` - starting point, implicitly added to the list of allowed states.
    pub fn new(initial_state: StateTag) -> Self {
        let allowed_states = vec![initial_state];
        Self {
            initial_state,
            allowed_states,
            allowed_transitions: Vec::new(),
        }
    }

    /// Add allowed state.
    pub fn add_state(mut self, state: StateTag) -> Self {
        self.add_state_internal(state);
        self
    }

    /// Add allowed transition.
    pub fn add_transition(mut self, transition: (StateTag, StateTag)) -> Self {
        self.add_transition_internal(transition);
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
        // Check number of transitions.
        if self.allowed_transitions.is_empty() {
            error!("No transitions have been added. LogicMonitor cannot be created.");
            return Err(HealthMonitorError::WrongState);
        }

        // Check transitions are between allowed states.
        for (from, to) in self.allowed_transitions.iter() {
            if !self.allowed_states.contains(from) {
                error!("Invalid transition definition - 'from' state is unknown: {:?}", from);
                return Err(HealthMonitorError::InvalidArgument);
            }
            if !self.allowed_states.contains(to) {
                error!("Invalid transition definition - 'to' state is unknown: {:?}", to);
                return Err(HealthMonitorError::InvalidArgument);
            }
        }

        let inner = Arc::new(LogicMonitorInner::new(
            monitor_tag,
            self.initial_state,
            self.allowed_states,
            self.allowed_transitions,
        ));
        Ok(LogicMonitor::new(inner))
    }

    // FFI internals.

    pub(crate) fn add_state_internal(&mut self, state: StateTag) {
        if !self.allowed_states.contains(&state) {
            self.allowed_states.push(state);
        }
    }

    pub(crate) fn add_transition_internal(&mut self, transition: (StateTag, StateTag)) {
        if !self.allowed_transitions.contains(&transition) {
            self.allowed_transitions.push(transition);
        }
    }
}

/// Logic monitor.
pub struct LogicMonitor {
    inner: Arc<LogicMonitorInner>,
}

impl LogicMonitor {
    /// Create a new [`LogicMonitor`] instance.
    fn new(inner: Arc<LogicMonitorInner>) -> Self {
        Self { inner }
    }

    /// Perform transition to a new state.
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

    /// Hashed current state.
    current_state: AtomicU64,

    /// State of the monitor.
    /// Contains zero for correct state.
    /// Contains [`LogicEvaluationError`] if in erroneous state.
    monitor_state: AtomicU8,

    /// List of allowed states.
    allowed_states: Vec<StateTag>,

    /// List of allowed transitions between states.
    allowed_transitions: Vec<(StateTag, StateTag)>,
}

impl MonitorEvaluator for LogicMonitorInner {
    fn evaluate(&self, _hmon_starting_point: Instant, on_error: &mut dyn FnMut(&MonitorTag, MonitorEvaluationError)) {
        let monitor_state = self.monitor_state.load(Ordering::Relaxed);
        if monitor_state != OK_STATE {
            let error = LogicEvaluationError::from(monitor_state);
            warn!("Invalid logic monitor state observed: {:?}", error);
            on_error(&self.monitor_tag, error.into());
        }
    }
}

impl LogicMonitorInner {
    fn new(
        monitor_tag: MonitorTag,
        initial_state: StateTag,
        allowed_states: Vec<StateTag>,
        allowed_transitions: Vec<(StateTag, StateTag)>,
    ) -> Self {
        let current_state = AtomicU64::new(HashedState::from(initial_state).into());
        let monitor_state = AtomicU8::new(0);
        LogicMonitorInner {
            monitor_tag,
            current_state,
            monitor_state,
            allowed_states,
            allowed_transitions,
        }
    }

    fn transition(&self, new_state: StateTag) -> Result<StateTag, LogicEvaluationError> {
        // Get current state.
        let current_state = self.state()?;

        // Check new state is valid.
        if !self.allowed_states.contains(&new_state) {
            // Move to `InvalidState` if requested state is not known.
            warn!("Requested state transition to unknown state: {:?}", new_state);
            let new_monitor_state = LogicEvaluationError::InvalidState;
            self.monitor_state.store(new_monitor_state.into(), Ordering::Relaxed);
            return Err(new_monitor_state);
        }

        // Check transition is valid.
        let transition = (current_state, new_state);
        if !self.allowed_transitions.contains(&transition) {
            // Move to `InvalidTransition` if requested transition is not known.
            warn!(
                "Requested state transition is invalid: {:?} -> {:?}",
                current_state, new_state
            );
            let new_monitor_state = LogicEvaluationError::InvalidTransition;
            self.monitor_state.store(new_monitor_state.into(), Ordering::Relaxed);
            return Err(new_monitor_state);
        }

        // Change state and return it.
        let hashed_new_state = HashedState::from(new_state);
        self.current_state.store(hashed_new_state.into(), Ordering::Relaxed);

        Ok(new_state)
    }

    fn state(&self) -> Result<StateTag, LogicEvaluationError> {
        // Current state cannot be determined.
        if self.monitor_state.load(Ordering::Relaxed) != OK_STATE {
            warn!("Current logic monitor state cannot be determined");
            return Err(LogicEvaluationError::InvalidState);
        }

        // Find current state.
        let hashed_state = HashedState::from(self.current_state.load(Ordering::Relaxed));
        let result = self
            .allowed_states
            .iter()
            .find(|e| HashedState::from(*e) == hashed_state);

        // Return current state if found.
        // `None` indicates logic error - it should not be possible to successfully change state into an unknown.
        match result {
            Some(state) => Ok(*state),
            None => Err(LogicEvaluationError::InvalidState),
        }
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
        let from_state = StateTag::from("from");
        let builder = LogicMonitorBuilder::new(from_state);
        assert_eq!(builder.initial_state, from_state);
        assert_eq!(builder.allowed_states, vec![from_state]);
        assert!(builder.allowed_transitions.is_empty());
    }

    #[test]
    fn logic_monitor_builder_build_succeeds() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let result = LogicMonitorBuilder::new(from_state)
            .add_state(to_state)
            .add_transition((from_state, to_state))
            .build(monitor_tag, &allocator);
        assert!(result.is_ok());
    }

    #[test]
    fn logic_monitor_builder_build_no_transitions() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let result = LogicMonitorBuilder::new(from_state).build(monitor_tag, &allocator);
        assert!(result.is_err_and(|e| e == HealthMonitorError::WrongState));
    }

    #[test]
    fn logic_monitor_builder_build_unknown_nodes() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");

        // Unknown "from".
        let result = LogicMonitorBuilder::new(from_state)
            .add_state(to_state)
            .add_transition((StateTag::from("unknown"), to_state))
            .build(monitor_tag, &allocator);
        assert!(result.is_err_and(|e| e == HealthMonitorError::InvalidArgument));

        // Unknown "to".
        let result = LogicMonitorBuilder::new(from_state)
            .add_state(to_state)
            .add_transition((from_state, StateTag::from("unknown")))
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
            .add_state(to_state)
            .add_transition((from_state, to_state))
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
            .add_state(to_state)
            .add_transition((from_state, to_state))
            .build(monitor_tag, &allocator)
            .unwrap();

        let result = monitor.transition(StateTag::from("unknown"));
        assert!(result.is_err_and(|e| e == LogicEvaluationError::InvalidState));
    }

    #[test]
    fn logic_monitor_transition_indeterminate_current_state() {
        let allocator = ProtectedMemoryAllocator {};
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let monitor = LogicMonitorBuilder::new(from_state)
            .add_state(to_state)
            .add_transition((from_state, to_state))
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
            .add_state(state2)
            .add_state(state3)
            .add_transition((state1, state2))
            .add_transition((state2, state3))
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
            .add_state(state2)
            .add_state(state3)
            .add_transition((state1, state2))
            .add_transition((state2, state3))
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
            .add_state(to_state)
            .add_transition((from_state, to_state))
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
        let hmon_starting_point = Instant::now();
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let monitor = LogicMonitorBuilder::new(from_state)
            .add_state(to_state)
            .add_transition((from_state, to_state))
            .build(monitor_tag, &allocator)
            .unwrap();

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
        let hmon_starting_point = Instant::now();
        let monitor_tag = MonitorTag::from("logic_monitor");
        let from_state = StateTag::from("from");
        let to_state = StateTag::from("to");
        let monitor = LogicMonitorBuilder::new(from_state)
            .add_state(to_state)
            .add_transition((from_state, to_state))
            .build(monitor_tag, &allocator)
            .unwrap();

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
                assert_eq!(error, LogicEvaluationError::InvalidState.into())
            });
        assert!(error_happened);
    }

    #[test]
    fn logic_monitor_evaluate_invalid_transition() {
        let allocator = ProtectedMemoryAllocator {};
        let hmon_starting_point = Instant::now();
        let monitor_tag = MonitorTag::from("logic_monitor");
        let state1 = StateTag::from("state1");
        let state2: StateTag = StateTag::from("state2");
        let state3 = StateTag::from("state3");
        let monitor = LogicMonitorBuilder::new(state1)
            .add_state(state2)
            .add_state(state3)
            .add_transition((state1, state2))
            .add_transition((state2, state3))
            .build(monitor_tag, &allocator)
            .unwrap();

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
