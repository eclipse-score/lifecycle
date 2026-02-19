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

use crate::log;
use core::fmt;
use core::hash::{Hash, Hasher};

/// Common string-based tag.
#[derive(Clone, Copy, Eq)]
#[repr(C)]
struct Tag {
    data: *const u8,
    length: usize,
}

unsafe impl Send for Tag {}
unsafe impl Sync for Tag {}

impl fmt::Debug for Tag {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // SAFETY: the underlying data was created from a valid `&str`.
        let bytes = unsafe { core::slice::from_raw_parts(self.data, self.length) };
        let s = unsafe { core::str::from_utf8_unchecked(bytes) };
        write!(f, "Tag({})", s)
    }
}

impl log::ScoreDebug for Tag {
    fn fmt(&self, f: log::Writer, _spec: &log::FormatSpec) -> Result<(), log::Error> {
        // SAFETY: the underlying data was created from a valid `&str`.
        let bytes = unsafe { core::slice::from_raw_parts(self.data, self.length) };
        let s = unsafe { core::str::from_utf8_unchecked(bytes) };
        log::score_write!(f, "Tag({})", s)
    }
}

impl Hash for Tag {
    fn hash<H: Hasher>(&self, state: &mut H) {
        // SAFETY: the underlying data was created from a valid `&str`.
        let bytes = unsafe { core::slice::from_raw_parts(self.data, self.length) };
        bytes.hash(state);
    }
}

impl PartialEq for Tag {
    fn eq(&self, other: &Self) -> bool {
        // SAFETY: the underlying data was created from a valid `&str`.
        let self_bytes = unsafe { core::slice::from_raw_parts(self.data, self.length) };
        let other_bytes = unsafe { core::slice::from_raw_parts(other.data, other.length) };
        self_bytes == other_bytes
    }
}

impl From<String> for Tag {
    fn from(value: String) -> Self {
        let leaked = value.leak();
        Self {
            data: leaked.as_ptr(),
            length: leaked.len(),
        }
    }
}

impl From<&str> for Tag {
    fn from(value: &str) -> Self {
        let leaked = value.to_string().leak();
        Self {
            data: leaked.as_ptr(),
            length: leaked.len(),
        }
    }
}

/// Monitor tag.
#[derive(Clone, Copy, Eq, Hash, PartialEq)]
#[repr(C)]
pub struct MonitorTag(Tag);

impl fmt::Debug for MonitorTag {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // SAFETY: the underlying data was created from a valid `&str`.
        let bytes = unsafe { core::slice::from_raw_parts(self.0.data, self.0.length) };
        let s = unsafe { core::str::from_utf8_unchecked(bytes) };
        write!(f, "MonitorTag({})", s)
    }
}

impl log::ScoreDebug for MonitorTag {
    fn fmt(&self, f: log::Writer, _spec: &log::FormatSpec) -> Result<(), log::Error> {
        // SAFETY: the underlying data was created from a valid `&str`.
        let bytes = unsafe { core::slice::from_raw_parts(self.0.data, self.0.length) };
        let s = unsafe { core::str::from_utf8_unchecked(bytes) };
        log::score_write!(f, "MonitorTag({})", s)
    }
}

impl From<String> for MonitorTag {
    fn from(value: String) -> Self {
        Self(Tag::from(value))
    }
}

impl From<&str> for MonitorTag {
    fn from(value: &str) -> Self {
        Self(Tag::from(value))
    }
}

/// Deadline tag.
#[derive(Clone, Copy, Eq, Hash, PartialEq)]
#[repr(C)]
pub struct DeadlineTag(Tag);

impl fmt::Debug for DeadlineTag {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // SAFETY: the underlying data was created from a valid `&str`.
        let bytes = unsafe { core::slice::from_raw_parts(self.0.data, self.0.length) };
        let s = unsafe { core::str::from_utf8_unchecked(bytes) };
        write!(f, "DeadlineTag({})", s)
    }
}

impl log::ScoreDebug for DeadlineTag {
    fn fmt(&self, f: log::Writer, _spec: &log::FormatSpec) -> Result<(), log::Error> {
        // SAFETY: the underlying data was created from a valid `&str`.
        let bytes = unsafe { core::slice::from_raw_parts(self.0.data, self.0.length) };
        let s = unsafe { core::str::from_utf8_unchecked(bytes) };
        log::score_write!(f, "DeadlineTag({})", s)
    }
}

impl From<String> for DeadlineTag {
    fn from(value: String) -> Self {
        Self(Tag::from(value))
    }
}

impl From<&str> for DeadlineTag {
    fn from(value: &str) -> Self {
        Self(Tag::from(value))
    }
}
