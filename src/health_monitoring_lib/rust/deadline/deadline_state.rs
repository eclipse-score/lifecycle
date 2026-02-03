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
use core::{
    fmt::Debug,
    sync::atomic::{AtomicU64, Ordering},
};

#[derive(Clone, Copy)]
pub(super) struct DeadlineStateSnapshot(u64);

// Deadline State layout (u64) = | timestamp: u32 | reserved: 28 bits | finished_too_early: 1 bit | reserved: 1 bit | stopped: 1 bit | running: 1 bit |
const DEADLINE_STATE_MASK: u64 = 0b0000_1111;
const DEADLINE_STATE_RUNNING: u64 = 0b0000_0010;
const DEADLINE_STATE_STOPPED: u64 = 0b0000_0001;
const DEADLINE_STATE_FINISHED_TOO_EARLY: u64 = 0b0000_1000;

impl DeadlineStateSnapshot {
    pub(super) fn new(val: u64) -> Self {
        Self(val)
    }

    pub(super) fn as_u64(&self) -> u64 {
        self.0
    }

    pub(super) fn is_running(&self) -> bool {
        (self.0 & DEADLINE_STATE_RUNNING) != 0
    }

    pub(super) fn is_stopped(&self) -> bool {
        (self.0 & DEADLINE_STATE_STOPPED) != 0
    }

    pub(super) fn is_underrun(&self) -> bool {
        (self.0 & DEADLINE_STATE_FINISHED_TOO_EARLY) != 0
    }

    /// Get timestamp in milliseconds. This is a offset from an start timer that is stored in DeadlineMonitor
    pub(super) fn timestamp_ms(&self) -> u32 {
        ((self.0 & !DEADLINE_STATE_MASK) >> u32::BITS) as u32
    }

    pub(super) fn set_timestamp_ms(&mut self, timestamp: u32) {
        self.0 = ((timestamp as u64) << u32::BITS) | (self.0 & DEADLINE_STATE_MASK);
    }

    pub(super) fn set_running(&mut self) {
        self.0 |= DEADLINE_STATE_RUNNING;
    }

    pub(super) fn set_underrun(&mut self) {
        self.0 |= DEADLINE_STATE_FINISHED_TOO_EARLY;
    }
}

impl Debug for DeadlineStateSnapshot {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_struct("DeadlineStateSnapshot")
            .field("timestamp", &self.timestamp_ms())
            .field("is_running", &self.is_running())
            .field("is_stopped", &self.is_stopped())
            .field("is_underrun", &self.is_underrun())
            .finish()
    }
}

impl crate::log::ScoreDebug for DeadlineStateSnapshot {
    fn fmt(&self, f: crate::log::Writer, spec: &crate::log::FormatSpec) -> Result<(), crate::log::Error> {
        crate::log::DebugStruct::new(f, spec, "DeadlineStateSnapshot")
            .field("timestamp", &self.timestamp_ms())
            .field("is_running", &self.is_running())
            .field("is_stopped", &self.is_stopped())
            .field("is_underrun", &self.is_underrun())
            .finish()
    }
}

impl Default for DeadlineStateSnapshot {
    fn default() -> Self {
        Self(DEADLINE_STATE_STOPPED)
    }
}

pub(super) struct DeadlineState(AtomicU64);

impl DeadlineState {
    /// Creates a new `DeadlineState` with the initial state set to stopped.
    pub(super) fn new() -> Self {
        Self(AtomicU64::new(DEADLINE_STATE_STOPPED))
    }

    /// Returns a snapshot of the current deadline state.
    pub(super) fn snapshot(&self) -> DeadlineStateSnapshot {
        DeadlineStateSnapshot::new(self.0.load(Ordering::Relaxed))
    }

    /// Updates the deadline state using the provided closure.
    /// The closure receives the current state snapshot and should return
    /// an Option with the new state snapshot. If None is returned, the state
    /// is not updated.
    pub(super) fn update<F: FnMut(DeadlineStateSnapshot) -> Option<DeadlineStateSnapshot>>(
        &self,
        mut f: F,
    ) -> Result<DeadlineStateSnapshot, DeadlineStateSnapshot> {
        // Prev values returned
        self.0
            .fetch_update(Ordering::Relaxed, Ordering::Relaxed, |prev| {
                let snapshot = DeadlineStateSnapshot::new(prev);
                f(snapshot).map(|new_snapshot| new_snapshot.as_u64())
            })
            .map(DeadlineStateSnapshot::new)
            .map_err(DeadlineStateSnapshot::new)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn deadline_state_default_and_snapshot() {
        let state = DeadlineState::new();
        let snap = state.snapshot();
        assert!(snap.is_stopped());
        assert!(!snap.is_running());
        assert!(!snap.is_underrun());
    }

    #[test]
    fn deadline_state_update_success() {
        let state = DeadlineState::new();
        // Set running and timestamp
        let res = state.update(|mut snap| {
            snap.set_running();
            snap.set_timestamp_ms(1234);
            Some(snap)
        });
        assert!(res.is_ok());
        let snap = state.snapshot();
        assert!(snap.is_running());
        assert_eq!(snap.timestamp_ms(), 1234);
    }

    #[test]
    fn deadline_state_update_none_returns_err() {
        let state = DeadlineState::new();
        // Closure returns None, so state should not change
        let res = state.update(|_snap| None);
        assert!(res.is_err());
        let snap = state.snapshot();
        assert!(snap.is_stopped());
    }

    #[test]
    fn default_state() {
        let snap = DeadlineStateSnapshot::default();
        assert!(snap.is_stopped());
        assert!(!snap.is_running());
        assert!(!snap.is_underrun());
        assert_eq!(snap.timestamp_ms(), 0);
    }

    #[test]
    fn set_and_get_timestamp_ms() {
        let mut snap = DeadlineStateSnapshot::default();
        snap.set_timestamp_ms(0x12345678);
        assert_eq!(snap.timestamp_ms(), 0x12345678);
        assert!(!snap.is_running());
        assert!(snap.is_stopped()); // Default is stopped, running is set as a flag
        assert!(!snap.is_underrun());
    }

    #[test]
    fn set_running() {
        let mut snap = DeadlineStateSnapshot::default();
        snap.set_running();
        assert!(snap.is_running());
        assert!(snap.is_stopped()); // Default is stopped, running is set as a flag
        assert!(!snap.is_underrun());
    }

    #[test]
    fn set_underrun() {
        let mut snap = DeadlineStateSnapshot::default();
        snap.set_underrun();
        assert!(snap.is_underrun());
        assert!(!snap.is_running());
        assert!(snap.is_stopped()); // Default is stopped, running is set as a flag
    }

    #[test]
    fn as_u64_and_new() {
        let mut snap = DeadlineStateSnapshot::default();
        snap.set_timestamp_ms(42);
        snap.set_running();
        let val = snap.as_u64();
        let snap2 = DeadlineStateSnapshot::new(val);
        assert_eq!(snap2.timestamp_ms(), 42);
        assert!(snap2.is_running());
    }
}
