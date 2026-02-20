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
/// - cycle start timestamp: 32 bits
/// - heartbeat timestamp offset: 29 bits
/// - heartbeat counter: 2 bits
/// - post-init flag: 1 bit
#[derive(Clone, Copy, Default)]
pub struct HeartbeatStateSnapshot(u64);

const START_MASK: u64 = 0xFFFFFFFF_00000000;
const START_OFFSET: u32 = u32::BITS;
const BEAT_MASK: u64 = 0x00000000_FFFFFFF8;
const BEAT_OFFSET: u32 = 3;
const COUNT_MASK: u64 = 0b0110;
const COUNT_OFFSET: u32 = 1;
const POST_INIT_MASK: u64 = 0b0001;

impl HeartbeatStateSnapshot {
    /// Create a new snapshot with known starting point.
    /// `post_init` flag is implicitly set to 1.
    pub fn new(start_timestamp: u32) -> Self {
        let mut snapshot = Self::default();
        snapshot.set_start_timestamp(start_timestamp);
        snapshot.set_post_init(true);
        snapshot
    }

    /// Return underlying data.
    pub fn as_u64(&self) -> u64 {
        self.0
    }

    /// Cycle start timestamp.
    pub fn start_timestamp(&self) -> u32 {
        ((self.0 & START_MASK) >> START_OFFSET) as u32
    }

    /// Set cycle start timestamp.
    pub fn set_start_timestamp(&mut self, value: u32) {
        self.0 = ((value as u64) << START_OFFSET) | (self.0 & !START_MASK);
    }

    /// Heartbeat timestamp offset.
    pub fn heartbeat_timestamp_offset(&self) -> u32 {
        ((self.0 & BEAT_MASK) >> BEAT_OFFSET) as u32
    }

    /// Set heartbeat timestamp offset.
    /// Value is 29-bit, must be lower than 0x1FFFFFFF.
    pub fn set_heartbeat_timestamp_offset(&mut self, value: u32) {
        assert!(value < 1 << 29, "provided heartbeat offset is out of range");
        self.0 = ((value as u64) << BEAT_OFFSET) | (self.0 & !BEAT_MASK);
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
    fn test_snapshot_new_zero() {
        let state = HeartbeatStateSnapshot::new(0);

        assert_eq!(state.as_u64(), 0x01);
        assert_eq!(state.start_timestamp(), 0);
        assert_eq!(state.heartbeat_timestamp_offset(), 0);
        assert_eq!(state.counter(), 0);
        assert!(state.post_init());
    }

    #[test]
    fn test_snapshot_new_valid() {
        let state = HeartbeatStateSnapshot::new(0xDEADBEEF);

        assert_eq!(state.as_u64(), (0xDEADBEEF << u32::BITS) + 0x01);
        assert_eq!(state.start_timestamp(), 0xDEADBEEF);
        assert_eq!(state.heartbeat_timestamp_offset(), 0);
        assert_eq!(state.counter(), 0);
        assert!(state.post_init());
    }

    #[test]
    fn test_snapshot_new_max() {
        let state = HeartbeatStateSnapshot::new(u32::MAX);

        assert_eq!(state.as_u64(), ((u32::MAX as u64) << u32::BITS) + 0x01);
        assert_eq!(state.start_timestamp(), u32::MAX);
        assert_eq!(state.heartbeat_timestamp_offset(), 0);
        assert_eq!(state.counter(), 0);
        assert!(state.post_init());
    }

    #[test]
    fn test_snapshot_from_u64_zero() {
        let state = HeartbeatStateSnapshot::from(0);

        assert_eq!(state.as_u64(), 0);
        assert_eq!(state.start_timestamp(), 0);
        assert_eq!(state.heartbeat_timestamp_offset(), 0);
        assert_eq!(state.counter(), 0);
        assert!(!state.post_init());
    }

    #[test]
    fn test_snapshot_from_u64_valid() {
        let state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);

        assert_eq!(state.as_u64(), 0xDEADBEEF_DEADBEEF);
        assert_eq!(state.start_timestamp(), 0xDEADBEEF);
        assert_eq!(state.heartbeat_timestamp_offset(), 0xDEADBEEF >> 3);
        assert_eq!(state.counter(), 3);
        assert!(state.post_init());
    }

    #[test]
    fn test_snapshot_from_u64_max() {
        let state = HeartbeatStateSnapshot::from(u64::MAX);

        assert_eq!(state.as_u64(), u64::MAX);
        assert_eq!(state.start_timestamp(), u32::MAX);
        assert_eq!(state.heartbeat_timestamp_offset(), u32::MAX >> 3);
        assert_eq!(state.counter(), 3);
        assert!(state.post_init());
    }

    #[test]
    fn test_snapshot_default() {
        let state = HeartbeatStateSnapshot::default();

        assert_eq!(state.as_u64(), 0);
        assert_eq!(state.start_timestamp(), 0);
        assert_eq!(state.heartbeat_timestamp_offset(), 0);
        assert_eq!(state.counter(), 0);
        assert!(!state.post_init());
    }

    #[test]
    fn test_snapshot_set_start_timestamp() {
        let mut state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);
        state.set_start_timestamp(0xCAFEBAAD);

        assert_eq!(state.start_timestamp(), 0xCAFEBAAD);

        // Check other parameters unchanged.
        assert_eq!(state.heartbeat_timestamp_offset(), 0xDEADBEEF >> 3);
        assert_eq!(state.counter(), 3);
        assert!(state.post_init());
    }

    #[test]
    fn test_snapshot_set_heartbeat_timestamp_valid() {
        let mut state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);
        state.set_heartbeat_timestamp_offset(0x1CAFEBAD);

        assert_eq!(state.heartbeat_timestamp_offset(), 0x1CAFEBAD);

        // Check other parameters unchanged.
        assert_eq!(state.start_timestamp(), 0xDEADBEEF);
        assert_eq!(state.counter(), 3);
        assert!(state.post_init());
    }

    #[test]
    #[should_panic(expected = "provided heartbeat offset is out of range")]
    fn test_snapshot_set_heartbeat_timestamp_out_of_range() {
        let mut state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);
        state.set_heartbeat_timestamp_offset(0x20000000);
    }

    #[test]
    fn test_snapshot_counter_increment() {
        let mut state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEE9);

        // Max value is 3, check if saturates.
        for i in 1..=4 {
            state.increment_counter();
            assert_eq!(state.counter(), min(i, 3));
        }

        // Check other parameters unchanged.
        assert_eq!(state.start_timestamp(), 0xDEADBEEF);
        assert_eq!(state.heartbeat_timestamp_offset(), 0xDEADBEEF >> 3);
        assert!(state.post_init());
    }

    #[test]
    fn test_snapshot_set_post_init() {
        let mut state = HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF);

        state.set_post_init(false);
        assert!(!state.post_init());
        state.set_post_init(true);
        assert!(state.post_init());

        // Check other parameters unchanged.
        assert_eq!(state.start_timestamp(), 0xDEADBEEF);
        assert_eq!(state.heartbeat_timestamp_offset(), 0xDEADBEEF >> 3);
        assert_eq!(state.counter(), 3);
    }

    #[test]
    fn test_state_new() {
        let state = HeartbeatState::new(HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF));
        assert_eq!(state.0.load(Ordering::Relaxed), 0xDEADBEEF_DEADBEEF);
    }

    #[test]
    fn test_state_snapshot() {
        let state = HeartbeatState::new(HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF));
        assert_eq!(state.snapshot().as_u64(), 0xDEADBEEF_DEADBEEF);
    }

    #[test]
    fn test_state_update_some() {
        let state = HeartbeatState::new(HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF));
        let _ = state.update(|prev_snapshot| {
            // Make sure state is as expected.
            assert_eq!(prev_snapshot.as_u64(), 0xDEADBEEF_DEADBEEF);
            assert_eq!(prev_snapshot.start_timestamp(), 0xDEADBEEF);
            assert_eq!(prev_snapshot.heartbeat_timestamp_offset(), 0xDEADBEEF >> 3);
            assert_eq!(prev_snapshot.counter(), 3);
            assert!(prev_snapshot.post_init());

            Some(HeartbeatStateSnapshot::from(0))
        });

        assert_eq!(state.snapshot().as_u64(), 0);
    }

    #[test]
    fn test_state_update_none() {
        let state = HeartbeatState::new(HeartbeatStateSnapshot::from(0xDEADBEEF_DEADBEEF));
        let _ = state.update(|_| None);

        assert_eq!(state.snapshot().as_u64(), 0xDEADBEEF_DEADBEEF);
    }
}
