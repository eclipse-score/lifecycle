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

use core::hash::Hash;
use core::time::Duration;
use std::time::Instant;

/// Range of accepted time.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct TimeRange {
    pub min: Duration,
    pub max: Duration,
}

impl TimeRange {
    pub fn new(min: Duration, max: Duration) -> Self {
        assert!(min <= max, "TimeRange min must be less than or equal to max");
        Self { min, max }
    }
}

/// Get offset between HMON and monitor starting time points as [`u32`].
pub(crate) fn hmon_time_offset(hmon_starting_point: Instant, monitor_starting_point: Instant) -> u32 {
    let result = hmon_starting_point.checked_duration_since(monitor_starting_point);
    let duration_since = result.expect("HMON starting point is earlier than monitor starting point");
    duration_to_u32(duration_since)
}

/// Get duration as [`u32`].
pub(crate) fn duration_to_u32(duration: Duration) -> u32 {
    let millis = duration.as_millis();
    u32::try_from(millis).expect("Monitor running for too long")
}

#[cfg(test)]
mod tests {
    use crate::common::{duration_to_u32, hmon_time_offset};
    use core::time::Duration;
    use std::time::Instant;

    #[test]
    fn test_hmon_time_offset_valid() {
        let monitor_starting_point = Instant::now();
        let hmon_starting_point = Instant::now();
        let offset = hmon_time_offset(hmon_starting_point, monitor_starting_point);
        // Allow small offset.
        assert!(offset < 10);
    }

    #[test]
    #[should_panic(expected = "HMON starting point is earlier than monitor starting point")]
    fn test_hmon_time_offset_wrong_order() {
        let hmon_starting_point = Instant::now();
        let monitor_starting_point = Instant::now();
        let _offset = hmon_time_offset(hmon_starting_point, monitor_starting_point);
    }

    #[test]
    #[should_panic(expected = "Monitor running for too long")]
    fn test_hmon_time_offset_diff_too_large() {
        const HUNDRED_DAYS_AS_SECS: u64 = 100 * 24 * 60 * 60;
        let monitor_starting_point = Instant::now();
        let hmon_starting_point = Instant::now()
            .checked_add(Duration::from_secs(HUNDRED_DAYS_AS_SECS))
            .unwrap();
        let _offset = hmon_time_offset(hmon_starting_point, monitor_starting_point);
    }

    #[test]
    fn test_duration_to_u32_valid() {
        let result = duration_to_u32(Duration::from_millis(1234));
        assert_eq!(result, 1234);
    }

    #[test]
    #[should_panic(expected = "Monitor running for too long")]
    fn test_duration_to_u32_too_large() {
        const HUNDRED_DAYS_AS_SECS: u64 = 100 * 24 * 60 * 60;
        let _result = duration_to_u32(Duration::from_secs(HUNDRED_DAYS_AS_SECS));
    }
}
