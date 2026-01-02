// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
//

use std::{hash::{DefaultHasher, Hash, Hasher}, time::Duration};

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
#[repr(C)]
pub enum Error {
    _NoError = 0, // Only used by the FFI API.
    BadParameter,
    DoesNotExist,
    NotAllowed,
    OutOfMemory,
    Generic,
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
#[repr(C)]
pub enum Status {
    Running = 0,
    Disabled,
    Failed,
    Stopped,
}

impl Into<u32> for Status {
    fn into(self) -> u32 {
        self as u32
    }
}

impl From<u32> for Status {
    fn from(value: u32) -> Self {
        match value {
            0 => Self::Running,
            1 => Self::Disabled,
            2 => Self::Failed,
            3 => Self::Stopped,
            _ => panic!("Trying to create Status from unknown value {}", value),
        }
    }
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
#[repr(C)]
pub struct Tag {
    pub(crate) hash: u64,
}

impl Tag {
    pub fn from_str<N: AsRef<str> + Hash>(name: N) -> Self {
        let mut hasher = DefaultHasher::new();
        name.hash(&mut hasher);

        Self {
            hash: hasher.finish(),
        }
    }
}

impl From<&str> for Tag {
    fn from(value: &str) -> Self {
        Self::from_str(value)
    }
}

impl Hash for Tag {
    fn hash<H: Hasher>(&self, state: &mut H) {
        state.write_u64(self.hash);
    }
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct DurationRange {
    pub min: Duration,
    pub max: Duration,
}

impl DurationRange {
    pub fn new(min: Duration, max: Duration) -> Self {
        Self { min, max }
    }

    pub fn from_millis(min_ms: u64, max_ms: u64) -> Self {
        Self::new(Duration::from_millis(min_ms), Duration::from_millis(max_ms))
    }
}
