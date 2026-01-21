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

use crate::log::*;
use core::time::Duration;

/// Unique identifier for deadlines.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, ScoreDebug)]
pub struct IdentTag(&'static str); // Internal representation as a leaked string slice for now. It can be also an str to u64 conversion. Since this is internal only, we can change it later if needed.

impl From<String> for IdentTag {
    fn from(value: String) -> Self {
        Self(value.leak())
    }
}

impl From<&str> for IdentTag {
    fn from(value: &str) -> Self {
        Self(value.to_string().leak())
    }
}

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
