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

use crate::common::{AtomicU64, Ordering};
use crate::logic::logic_monitor::OK_STATE;
use crate::logic::LogicEvaluationError;

/// Snapshot of a logic state.
/// Layout (u64) = | current state index: 56 bits | monitor status: u8 |
#[derive(Clone, Copy)]
pub struct LogicStateSnapshot(u64);

const INDEX_MASK: u64 = 0xFFFFFFFF_FFFFFF00;
const INDEX_OFFSET: u32 = u8::BITS;
const STATUS_MASK: u64 = 0xFF;

impl LogicStateSnapshot {
    /// Create a new snapshot.
    pub fn new(initial_state_index: usize) -> Self {
        let mut snapshot = Self(0);
        snapshot.set_current_state_index(initial_state_index);
        snapshot
    }

    /// Return underlying data.
    pub fn as_u64(&self) -> u64 {
        self.0
    }

    /// Current state index.
    pub fn current_state_index(&self) -> usize {
        ((self.0 & INDEX_MASK) >> INDEX_OFFSET) as usize
    }

    /// Set current state index.
    /// Value is 56-bit, max accepted value is 0x00FFFFFF_FFFFFFFF.
    pub fn set_current_state_index(&mut self, value: usize) {
        assert!(value < 1 << 56, "provided state index is out of range");
        self.0 = ((value as u64) << INDEX_OFFSET) | (self.0 & !INDEX_MASK)
    }

    /// Monitor status.
    /// - zero if healthy.
    /// - `LogicEvaluationError` if not.
    pub fn monitor_status(&self) -> Result<(), LogicEvaluationError> {
        let value = (self.0 & STATUS_MASK) as u8;
        if value == OK_STATE {
            Ok(())
        } else {
            Err(value.try_into().map_err(|_| LogicEvaluationError::UnmappedError)?)
        }
    }

    /// Set monitor status.
    pub fn set_monitor_status(&mut self, value: LogicEvaluationError) {
        self.0 = (value as u64) | (self.0 & !STATUS_MASK);
    }
}

impl From<u64> for LogicStateSnapshot {
    fn from(value: u64) -> Self {
        Self(value)
    }
}

/// Atomic representation of [`LogicStateSnapshot`].
pub struct LogicState(AtomicU64);

impl LogicState {
    /// Create a new [`LogicState`].
    pub fn new(initial_state_index: usize) -> Self {
        let snapshot = LogicStateSnapshot::new(initial_state_index);
        Self(AtomicU64::new(snapshot.as_u64()))
    }

    /// Return a snapshot of the current logic state.
    #[allow(dead_code)]
    pub fn snapshot(&self) -> LogicStateSnapshot {
        LogicStateSnapshot::from(self.0.load(Ordering::Acquire))
    }

    /// Store a new [`LogicStateSnapshot`] and return the previous one.
    pub fn swap(&self, new: LogicStateSnapshot) -> LogicStateSnapshot {
        self.0.swap(new.as_u64(), Ordering::AcqRel).into()
    }
}

#[cfg(all(test, not(loom)))]
mod tests {
    use crate::logic::logic_state::{LogicState, LogicStateSnapshot};
    use crate::logic::LogicEvaluationError;
    use core::sync::atomic::Ordering;

    #[test]
    fn snapshot_new_succeeds() {
        let initial_state_index = 4321;
        let state = LogicStateSnapshot::new(initial_state_index);

        assert_eq!(state.as_u64(), (initial_state_index as u64) << u8::BITS);
        assert_eq!(state.current_state_index(), initial_state_index);
        assert!(state.monitor_status().is_ok());
    }

    #[test]
    fn snapshot_from_u64_zero() {
        let state = LogicStateSnapshot::from(0);

        assert_eq!(state.as_u64(), 0x00);
        assert_eq!(state.current_state_index(), 0);
        assert!(state.monitor_status().is_ok());
    }

    #[test]
    fn snapshot_from_u64_valid() {
        let state = LogicStateSnapshot::from(0xDEADBEEF_DEADBE01);

        assert_eq!(state.as_u64(), 0xDEADBEEF_DEADBE01);
        assert_eq!(state.current_state_index(), 0xDEADBEEF_DEADBE01 >> u8::BITS);
        assert!(state
            .monitor_status()
            .is_err_and(|e| e == LogicEvaluationError::InvalidState));
    }

    #[test]
    fn snapshot_from_u64_max() {
        let state = LogicStateSnapshot::from(u64::MAX);

        assert_eq!(state.as_u64(), u64::MAX);
        assert_eq!(state.current_state_index(), (u64::MAX >> u8::BITS) as usize);
        assert!(state
            .monitor_status()
            .is_err_and(|e| e == LogicEvaluationError::UnmappedError));
    }

    #[test]
    fn snapshot_set_current_state_index_valid() {
        let mut state = LogicStateSnapshot::from(0xDEADBEEF_DEADBE00);
        state.set_current_state_index(0x00CAFEBA_DCAFEBAD);

        assert_eq!(state.current_state_index(), 0x00CAFEBA_DCAFEBAD);

        // Check other parameters unchanged.
        assert!(state.monitor_status().is_ok());
    }

    #[test]
    #[should_panic(expected = "provided state index is out of range")]
    fn snapshot_set_heartbeat_timestamp_out_of_range() {
        let mut state = LogicStateSnapshot::from(0xDEADBEEF_DEADBEEF);
        state.set_current_state_index(0x01000000_00000000);
    }

    #[test]
    fn snapshot_set_monitor_status_valid() {
        let mut state = LogicStateSnapshot::from(0xDEADBEEF_DEADBEEF);
        state.set_monitor_status(LogicEvaluationError::InvalidTransition);

        assert!(state
            .monitor_status()
            .is_err_and(|e| e == LogicEvaluationError::InvalidTransition));

        // Check other parameters unchanged.
        assert_eq!(state.current_state_index(), 0xDEADBEEF_DEADBEEF >> u8::BITS);
    }

    #[test]
    fn state_new() {
        let initial_state_index = 4321;
        let state = LogicState::new(initial_state_index);
        assert_eq!(
            state.0.load(Ordering::Relaxed),
            (initial_state_index as u64) << u8::BITS
        );
    }

    #[test]
    fn state_snapshot() {
        let initial_state_index = 4321;
        let state = LogicState::new(initial_state_index);
        assert_eq!(state.snapshot().as_u64(), (initial_state_index as u64) << u8::BITS);
    }

    #[test]
    fn state_swap() {
        let state = LogicState::new(0);
        let prev_snapshot = state.swap(LogicStateSnapshot::from(0xDEADBEEF_DEADBE02));

        assert_eq!(prev_snapshot.as_u64(), 0x00);
        assert_eq!(prev_snapshot.current_state_index(), 0);
        assert!(prev_snapshot.monitor_status().is_ok());

        let curr_snapshot = state.snapshot();
        assert_eq!(curr_snapshot.as_u64(), 0xDEADBEEF_DEADBE02);
        assert_eq!(curr_snapshot.current_state_index(), 0xDEADBEEF_DEADBE02 >> u8::BITS);
        assert!(curr_snapshot
            .monitor_status()
            .is_err_and(|e| e == LogicEvaluationError::InvalidTransition));
    }
}
