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
use core::cmp::min;

/// Snapshot of a heartbeat state.
/// Layout (u64) = | heartbeat timestamp: 62 bits | heartbeat counter: 2 bits |
#[derive(Clone, Copy, Default)]
pub struct HeartbeatStateSnapshot(u64);

const BEAT_MASK: u64 = 0xFFFFFFFF_FFFFFFFC;
const BEAT_OFFSET: u32 = 2;
const COUNT_MASK: u64 = 0b0011;

impl HeartbeatStateSnapshot {
    /// Create a new snapshot.
    pub fn new() -> Self {
        Self(0)
    }

    /// Return underlying data.
    pub fn as_u64(&self) -> u64 {
        self.0
    }

    /// Heartbeat timestamp.
    pub fn heartbeat_timestamp(&self) -> u64 {
        (self.0 & BEAT_MASK) >> BEAT_OFFSET
    }

    /// Set heartbeat timestamp.
    /// Value is 62-bit, max accepted value is 0x3FFFFFFF_FFFFFFFF.
    pub fn set_heartbeat_timestamp(&mut self, value: u64) {
        assert!(value < 1 << 62, "provided heartbeat offset is out of range");
        self.0 = (value << BEAT_OFFSET) | (self.0 & !BEAT_MASK);
    }

    /// Heartbeat counter.
    pub fn counter(&self) -> u8 {
        (self.0 & COUNT_MASK) as u8
    }

    /// Increment heartbeat counter.
    /// Value is 2-bit, larger values are saturated to max value (3).
    pub fn increment_counter(&mut self) {
        let value = min(self.counter() + 1, 3);
        self.0 = (value as u64) | (self.0 & !COUNT_MASK);
    }
}

impl From<u64> for HeartbeatStateSnapshot {
    fn from(value: u64) -> Self {
        Self(value)
    }
}

/// Atomic representation of [`HeartbeatStateSnapshot`].
#[derive(Default)]
pub struct HeartbeatState(AtomicU64);

impl HeartbeatState {
    /// Create a new [`HeartbeatState`] in a default zeroed state.
    pub fn new() -> Self {
        Self(AtomicU64::default())
    }

    /// Return a snapshot of the current heartbeat state.
    #[allow(dead_code)]
    pub fn snapshot(&self) -> HeartbeatStateSnapshot {
        HeartbeatStateSnapshot::from(self.0.load(Ordering::Acquire))
    }

    /// Update the heartbeat state using the provided closure.
    /// Closure receives the current state and should return an [`Option`] containing a new state.
    /// If [`None`] is returned then the state was not updated.
    pub fn update<F: FnMut(HeartbeatStateSnapshot) -> Option<HeartbeatStateSnapshot>>(
        &self,
        mut f: F,
    ) -> Result<HeartbeatStateSnapshot, HeartbeatStateSnapshot> {
        self.0
            .fetch_update(Ordering::AcqRel, Ordering::Acquire, |prev| {
                let snapshot = HeartbeatStateSnapshot::from(prev);
                f(snapshot).map(|new_snapshot: HeartbeatStateSnapshot| new_snapshot.as_u64())
            })
            .map(HeartbeatStateSnapshot::from)
            .map_err(HeartbeatStateSnapshot::from)
    }

    /// Reset the heartbeat state, returning the previous one.
    pub fn reset(&self) -> HeartbeatStateSnapshot {
        self.0
            .swap(HeartbeatStateSnapshot::new().as_u64(), Ordering::AcqRel)
            .into()
    }
}

#[cfg(all(test, not(loom)))]
mod tests {
    use crate::heartbeat::heartbeat_state::{HeartbeatState, HeartbeatStateSnapshot, BEAT_OFFSET};
    use core::cmp::min;
    use core::sync::atomic::Ordering;

    #[test]
    fn snapshot_new_succeeds() {
        let state = HeartbeatStateSnapshot::new();

        assert_eq!(state.as_u64(), 0x00);
        assert_eq!(state.heartbeat_timestamp(), 0);
        assert_eq!(state.counter(), 0);
    }

    #[test]
    fn snapshot_from_u64_zero() {
        let state = HeartbeatStateSnapshot::from(0);

        assert_eq!(state.as_u64(), 0x00);
        assert_eq!(state.heartbeat_timestamp(), 0);
        assert_eq!(state.counter(), 0);
    }

    #[test]
    fn snapshot_from_u64_valid() {
        let state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);

        assert_eq!(state.as_u64(), 0xDEADBEEF_DEADBEEF);
        assert_eq!(state.heartbeat_timestamp(), 0xDEADBEEF_DEADBEEF >> BEAT_OFFSET);
        assert_eq!(state.counter(), 3);
    }

    #[test]
    fn snapshot_from_u64_max() {
        let state = HeartbeatStateSnapshot::from(u64::MAX);

        assert_eq!(state.as_u64(), u64::MAX);
        assert_eq!(state.heartbeat_timestamp(), u64::MAX >> BEAT_OFFSET);
        assert_eq!(state.counter(), 3);
    }

    #[test]
    fn snapshot_default() {
        let state = HeartbeatStateSnapshot::default();

        assert_eq!(state.as_u64(), 0x00);
        assert_eq!(state.heartbeat_timestamp(), 0);
        assert_eq!(state.counter(), 0);
    }

    #[test]
    fn snapshot_set_heartbeat_timestamp_valid() {
        let mut state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);
        state.set_heartbeat_timestamp(0x3CAFEBAD_CAFEBAAD);

        assert_eq!(state.heartbeat_timestamp(), 0x3CAFEBAD_CAFEBAAD);

        // Check other parameters unchanged.
        assert_eq!(state.counter(), 3);
    }

    #[test]
    #[should_panic(expected = "provided heartbeat offset is out of range")]
    fn snapshot_set_heartbeat_timestamp_out_of_range() {
        let mut state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);
        state.set_heartbeat_timestamp(0x40000000_00000000);
    }

    #[test]
    fn snapshot_counter_increment() {
        let mut state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEC);

        // Max value is 3, check if saturates.
        for i in 1..=4 {
            state.increment_counter();
            assert_eq!(state.counter(), min(i, 3));
        }

        // Check other parameters unchanged.
        assert_eq!(state.heartbeat_timestamp(), 0xDEADBEEF_DEADBEEC >> BEAT_OFFSET);
    }

    #[test]
    fn state_new() {
        let state = HeartbeatState::new();
        assert_eq!(state.0.load(Ordering::Relaxed), 0x00);
    }

    #[test]
    fn state_default() {
        let state = HeartbeatState::default();
        assert_eq!(state.0.load(Ordering::Relaxed), 0x00);
    }

    #[test]
    fn state_snapshot() {
        let state = HeartbeatState::new();
        let _ = state.update(|_| Some(HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF)));
        assert_eq!(state.snapshot().as_u64(), 0xDEADBEEF_DEADBEEF);
    }

    #[test]
    fn state_update_some() {
        let state = HeartbeatState::new();
        let _ = state.update(|prev_snapshot| {
            // Make sure state is as expected.
            assert_eq!(prev_snapshot.as_u64(), 0x00);
            assert_eq!(prev_snapshot.heartbeat_timestamp(), 0);
            assert_eq!(prev_snapshot.counter(), 0);

            Some(HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF))
        });

        let _ = state.update(|prev_snapshot| {
            // Make sure state is as expected.
            assert_eq!(prev_snapshot.as_u64(), 0xDEADBEEF_DEADBEEF);
            assert_eq!(prev_snapshot.heartbeat_timestamp(), 0xDEADBEEF_DEADBEEF >> BEAT_OFFSET);
            assert_eq!(prev_snapshot.counter(), 3);

            Some(HeartbeatStateSnapshot::from(0))
        });

        assert_eq!(state.snapshot().as_u64(), 0);
    }

    #[test]
    fn state_update_none() {
        let state = HeartbeatState::new();
        let _ = state.update(|_| Some(HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF)));
        let _ = state.update(|_| None);

        assert_eq!(state.snapshot().as_u64(), 0xDEADBEEF_DEADBEEF);
    }

    #[test]
    fn state_reset() {
        let state = HeartbeatState::new();
        let snapshot_initial = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);
        let _ = state.update(|_| Some(snapshot_initial));

        let snapshot_from_reset = state.reset();
        let snapshot_after_reset = state.snapshot();

        assert_eq!(snapshot_initial.as_u64(), snapshot_from_reset.as_u64());
        assert_eq!(snapshot_after_reset.as_u64(), 0);
    }
}
