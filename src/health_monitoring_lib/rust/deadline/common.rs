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
    ops::Deref,
    sync::atomic::{AtomicBool, Ordering},
};

use crate::TimeRange;

/// Index type for identifying states associated with deadlines.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub(super) struct StateIndex(usize);

impl StateIndex {
    pub fn new(index: usize) -> Self {
        Self(index)
    }
}

impl Deref for StateIndex {
    type Target = usize;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

/// Template for a deadline, managing its range and usage state.
pub(super) struct DeadlineTemplate {
    range: TimeRange,
    is_in_use: AtomicBool,
    pub assigned_state_index: StateIndex,
}

impl DeadlineTemplate {
    pub(super) fn new(range: TimeRange, state_index: StateIndex) -> Self {
        Self {
            range,
            is_in_use: AtomicBool::new(false),
            assigned_state_index: state_index,
        }
    }

    /// Attempts to acquire the deadline for use.
    /// Returns Some(TimeRange) if successful, None if already in use.
    pub(super) fn acquire_deadline(&self) -> Option<TimeRange> {
        if self
            .is_in_use
            .compare_exchange(false, true, Ordering::Relaxed, Ordering::Relaxed)
            .is_ok()
        {
            Some(self.range)
        } else {
            None
        }
    }

    /// Releases the deadline, marking it as not in use.
    pub(super) fn release_deadline(&self) {
        self.is_in_use.store(false, Ordering::Relaxed);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::Arc;

    use crate::TimeRange;
    use core::time::Duration;

    #[test]
    fn new_and_fields() {
        let range = TimeRange::new(Duration::from_secs(1), Duration::from_secs(2));
        let idx = StateIndex::new(7);
        let tmpl = DeadlineTemplate::new(range, idx);
        assert_eq!(tmpl.range.min, Duration::from_secs(1));
        assert_eq!(tmpl.range.max, Duration::from_secs(2));
        assert_eq!(*tmpl.assigned_state_index, 7);
        assert!(!tmpl.is_in_use.load(Ordering::Relaxed));
    }

    #[test]
    fn acquire_and_release_deadline() {
        let range = TimeRange::new(Duration::from_secs(3), Duration::from_secs(4));
        let idx = StateIndex::new(0);
        let tmpl = Arc::new(DeadlineTemplate::new(range, idx));

        // First acquire should succeed
        assert_eq!(tmpl.acquire_deadline(), Some(range));
        // Second acquire should fail
        assert_eq!(tmpl.acquire_deadline(), None);

        // Release and acquire again
        tmpl.release_deadline();
        assert_eq!(tmpl.acquire_deadline(), Some(range));
    }

    #[test]
    fn concurrent_acquire() {
        use std::thread;
        let range = TimeRange::new(Duration::from_secs(5), Duration::from_secs(6));
        let idx = StateIndex::new(1);
        let tmpl = Arc::new(DeadlineTemplate::new(range, idx));

        let tmpl1 = tmpl.clone();
        let tmpl2 = tmpl.clone();
        let h1 = thread::spawn(move || tmpl1.acquire_deadline());
        let h2 = thread::spawn(move || tmpl2.acquire_deadline());
        let r1 = h1.join().unwrap();
        let r2 = h2.join().unwrap();
        // Only one thread should succeed
        assert!(matches!((r1, r2), (Some(_), None) | (None, Some(_))));
    }
}
