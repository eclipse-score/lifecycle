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

use core::cmp::min;

#[cfg(not(loom))]
use core::sync::atomic::{AtomicU64, Ordering};
#[cfg(loom)]
use loom::sync::atomic::{AtomicU64, Ordering};

/// Snapshot of a heartbeat state.
/// Data layout:
/// - heartbeat timestamp: 61 bits
/// - heartbeat counter: 2 bits
/// - post-init flag: 1 bit
#[derive(Clone, Copy, Default)]
pub struct HeartbeatStateSnapshot(u64);

const BEAT_MASK: u64 = 0xFFFFFFFF_FFFFFFF8;
const BEAT_OFFSET: u32 = 3;
const COUNT_MASK: u64 = 0b0110;
const COUNT_OFFSET: u32 = 1;
const POST_INIT_MASK: u64 = 0b0001;

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
    /// Value is 61-bit, must be lower than 0x1FFFFFFF_FFFFFFFF.
    pub fn set_heartbeat_timestamp(&mut self, value: u64) {
        assert!(value < 1 << 61, "provided heartbeat offset is out of range");
        self.0 = (value << BEAT_OFFSET) | (self.0 & !BEAT_MASK);
    }

    /// Heartbeat counter.
    pub fn counter(&self) -> u8 {
        ((self.0 & COUNT_MASK) >> COUNT_OFFSET) as u8
    }

    /// Increment heartbeat counter.
    /// Value is 2-bit, larger values are saturated to max value (3).
    pub fn increment_counter(&mut self) {
        let value = min(self.counter() + 1, 3);
        self.0 = ((value as u64) << COUNT_OFFSET) | (self.0 & !COUNT_MASK);
    }

    /// Post-init state.
    /// This should be `false` only before first cycle is concluded.
    pub fn post_init(&self) -> bool {
        let value = self.0 & POST_INIT_MASK;
        value != 0
    }

    /// Set post-init state.
    pub fn set_post_init(&mut self, value: bool) {
        self.0 = (value as u64) | (self.0 & !POST_INIT_MASK);
    }
}

impl From<u64> for HeartbeatStateSnapshot {
    fn from(value: u64) -> Self {
        Self(value)
    }
}

/// Atomic representation of [`HeartbeatStateSnapshot`].
pub struct HeartbeatState(AtomicU64);

impl HeartbeatState {
    /// Create a new [`HeartbeatState`] using provided [`HeartbeatStateSnapshot`].
    pub fn new(snapshot: HeartbeatStateSnapshot) -> Self {
        Self(AtomicU64::new(snapshot.as_u64()))
    }

    /// Return a snapshot of the current heartbeat state.
    pub fn snapshot(&self) -> HeartbeatStateSnapshot {
        HeartbeatStateSnapshot::from(self.0.load(Ordering::Relaxed))
    }

    /// Update the heartbeat state using the provided closure.
    /// Closure receives the current state and should return an [`Option`] containing a new state.
    /// If [`None`] is returned then the state was not updated.
    pub fn update<F: FnMut(HeartbeatStateSnapshot) -> Option<HeartbeatStateSnapshot>>(
        &self,
        mut f: F,
    ) -> Result<HeartbeatStateSnapshot, HeartbeatStateSnapshot> {
        // Prev values returned
        self.0
            .fetch_update(Ordering::Relaxed, Ordering::Relaxed, |prev| {
                let snapshot = HeartbeatStateSnapshot::from(prev);
                f(snapshot).map(|new_snapshot| new_snapshot.as_u64())
            })
            .map(HeartbeatStateSnapshot::from)
            .map_err(HeartbeatStateSnapshot::from)
    }
}

#[cfg(all(test, not(loom)))]
mod tests {
    use crate::heartbeat::heartbeat_state::{HeartbeatState, HeartbeatStateSnapshot};
    use core::cmp::min;
    use core::sync::atomic::Ordering;

    #[test]
    fn snapshot_new_succeeds() {
        let state = HeartbeatStateSnapshot::new();

        assert_eq!(state.as_u64(), 0x00);
        assert_eq!(state.heartbeat_timestamp(), 0);
        assert_eq!(state.counter(), 0);
        assert!(!state.post_init());
    }

    #[test]
    fn snapshot_from_u64_zero() {
        let state = HeartbeatStateSnapshot::from(0);

        assert_eq!(state.as_u64(), 0x00);
        assert_eq!(state.heartbeat_timestamp(), 0);
        assert_eq!(state.counter(), 0);
        assert!(!state.post_init());
    }

    #[test]
    fn snapshot_from_u64_valid() {
        let state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);

        assert_eq!(state.as_u64(), 0xDEADBEEF_DEADBEEF);
        assert_eq!(state.heartbeat_timestamp(), 0xDEADBEEF_DEADBEEF >> 3);
        assert_eq!(state.counter(), 3);
        assert!(state.post_init());
    }

    #[test]
    fn snapshot_from_u64_max() {
        let state = HeartbeatStateSnapshot::from(u64::MAX);

        assert_eq!(state.as_u64(), u64::MAX);
        assert_eq!(state.heartbeat_timestamp(), u64::MAX >> 3);
        assert_eq!(state.counter(), 3);
        assert!(state.post_init());
    }

    #[test]
    fn snapshot_default() {
        let state = HeartbeatStateSnapshot::default();

        assert_eq!(state.as_u64(), 0x00);
        assert_eq!(state.heartbeat_timestamp(), 0);
        assert_eq!(state.counter(), 0);
        assert!(!state.post_init());
    }

    #[test]
    fn snapshot_set_heartbeat_timestamp_valid() {
        let mut state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);
        state.set_heartbeat_timestamp(0x1CAFEBAD_CAFEBAAD);

        assert_eq!(state.heartbeat_timestamp(), 0x1CAFEBAD_CAFEBAAD);

        // Check other parameters unchanged.
        assert_eq!(state.counter(), 3);
        assert!(state.post_init());
    }

    #[test]
    #[should_panic(expected = "provided heartbeat offset is out of range")]
    fn snapshot_set_heartbeat_timestamp_out_of_range() {
        let mut state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);
        state.set_heartbeat_timestamp(0x20000000_00000000);
    }

    #[test]
    fn snapshot_counter_increment() {
        let mut state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEE9);

        // Max value is 3, check if saturates.
        for i in 1..=4 {
            state.increment_counter();
            assert_eq!(state.counter(), min(i, 3));
        }

        // Check other parameters unchanged.
        assert_eq!(state.heartbeat_timestamp(), 0xDEADBEEF_DEADBEE9 >> 3);
        assert!(state.post_init());
    }

    #[test]
    fn snapshot_set_post_init() {
        let mut state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);

        state.set_post_init(false);
        assert!(!state.post_init());
        state.set_post_init(true);
        assert!(state.post_init());

        // Check other parameters unchanged.
        assert_eq!(state.heartbeat_timestamp(), 0xDEADBEEF_DEADBEEF >> 3);
        assert_eq!(state.counter(), 3);
    }

    #[test]
    fn state_new() {
        let state = HeartbeatState::new(HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF));
        assert_eq!(state.0.load(Ordering::Relaxed), 0xDEADBEEF_DEADBEEF);
    }

    #[test]
    fn state_snapshot() {
        let state = HeartbeatState::new(HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF));
        assert_eq!(state.snapshot().as_u64(), 0xDEADBEEF_DEADBEEF);
    }

    #[test]
    fn state_update_some() {
        let state = HeartbeatState::new(HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF));
        let _ = state.update(|prev_snapshot| {
            // Make sure state is as expected.
            assert_eq!(prev_snapshot.as_u64(), 0xDEADBEEF_DEADBEEF);
            assert_eq!(prev_snapshot.heartbeat_timestamp(), 0xDEADBEEF_DEADBEEF >> 3);
            assert_eq!(prev_snapshot.counter(), 3);
            assert!(prev_snapshot.post_init());

            Some(HeartbeatStateSnapshot::from(0))
        });

        assert_eq!(state.snapshot().as_u64(), 0);
    }

    #[test]
    fn state_update_none() {
        let state = HeartbeatState::new(HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF));
        let _ = state.update(|_| None);

        assert_eq!(state.snapshot().as_u64(), 0xDEADBEEF_DEADBEEF);
    }
}
