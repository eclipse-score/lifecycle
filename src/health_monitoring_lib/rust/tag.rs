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

#[cfg(test)]
mod tests {
    use crate::log::score_write;
    use crate::tag::{DeadlineTag, MonitorTag, Tag};
    use core::fmt::Write;
    use core::hash::{Hash, Hasher};
    use score_log::fmt::{Error, FormatSpec, Result as FmtResult, ScoreWrite};
    use std::hash::DefaultHasher;

    struct StringWriter {
        buffer: String,
    }

    impl StringWriter {
        pub fn new() -> Self {
            Self { buffer: String::new() }
        }

        pub fn get(&self) -> &str {
            self.buffer.as_str()
        }
    }

    impl ScoreWrite for StringWriter {
        fn write_bool(&mut self, v: &bool, _spec: &FormatSpec) -> FmtResult {
            write!(self.buffer, "{}", v).map_err(|_| Error)
        }

        fn write_f32(&mut self, v: &f32, _spec: &FormatSpec) -> FmtResult {
            write!(self.buffer, "{}", v).map_err(|_| Error)
        }

        fn write_f64(&mut self, v: &f64, _spec: &FormatSpec) -> FmtResult {
            write!(self.buffer, "{}", v).map_err(|_| Error)
        }

        fn write_i8(&mut self, v: &i8, _spec: &FormatSpec) -> FmtResult {
            write!(self.buffer, "{}", v).map_err(|_| Error)
        }

        fn write_i16(&mut self, v: &i16, _spec: &FormatSpec) -> FmtResult {
            write!(self.buffer, "{}", v).map_err(|_| Error)
        }

        fn write_i32(&mut self, v: &i32, _spec: &FormatSpec) -> FmtResult {
            write!(self.buffer, "{}", v).map_err(|_| Error)
        }

        fn write_i64(&mut self, v: &i64, _spec: &FormatSpec) -> FmtResult {
            write!(self.buffer, "{}", v).map_err(|_| Error)
        }

        fn write_u8(&mut self, v: &u8, _spec: &FormatSpec) -> FmtResult {
            write!(self.buffer, "{}", v).map_err(|_| Error)
        }

        fn write_u16(&mut self, v: &u16, _spec: &FormatSpec) -> FmtResult {
            write!(self.buffer, "{}", v).map_err(|_| Error)
        }

        fn write_u32(&mut self, v: &u32, _spec: &FormatSpec) -> FmtResult {
            write!(self.buffer, "{}", v).map_err(|_| Error)
        }

        fn write_u64(&mut self, v: &u64, _spec: &FormatSpec) -> FmtResult {
            write!(self.buffer, "{}", v).map_err(|_| Error)
        }

        fn write_str(&mut self, v: &str, _spec: &FormatSpec) -> FmtResult {
            write!(self.buffer, "{}", v).map_err(|_| Error)
        }
    }

    fn compare_tag(tag: Tag, expected: &str) {
        let tag_as_bytes = unsafe { core::slice::from_raw_parts(tag.data, tag.length) };
        let tag_as_str = unsafe { core::str::from_utf8_unchecked(tag_as_bytes) };
        assert_eq!(tag_as_str, expected);
    }

    #[test]
    fn tag_debug() {
        let example_str = "EXAMPLE";
        let tag = Tag::from(example_str.to_string());
        assert_eq!(format!("{:?}", tag), "Tag(EXAMPLE)");
    }

    #[test]
    fn tag_score_debug() {
        let example_str = "EXAMPLE";
        let tag = Tag::from(example_str.to_string());
        let mut writer = StringWriter::new();
        assert!(score_write!(&mut writer, "{:?}", tag).is_ok());
        assert_eq!(writer.get(), "Tag(EXAMPLE)");
    }

    #[test]
    fn tag_hash() {
        let example_str = "EXAMPLE";
        let tag = Tag::from(example_str.to_string());
        let mut hasher = DefaultHasher::new();
        tag.hash(&mut hasher);
        let hash = hasher.finish();
        assert_eq!(hash, 14738755424381306335);
    }

    #[test]
    fn tag_partial_eq_is_eq() {
        let tag1 = Tag::from("same");
        let tag2 = Tag::from("same");
        assert_eq!(tag1, tag2);
    }

    #[test]
    fn tag_partial_eq_is_ne() {
        let tag1 = Tag::from("first");
        let tag2 = Tag::from("second");
        assert_ne!(tag1, tag2);
    }

    #[test]
    fn test_from_string() {
        let example_str = "EXAMPLE";
        let tag = Tag::from(example_str.to_string());
        compare_tag(tag, example_str);
    }

    #[test]
    fn test_from_str() {
        let example_str = "EXAMPLE";
        let tag = Tag::from(example_str);
        compare_tag(tag, example_str);
    }

    #[test]
    fn monitor_tag_debug() {
        let example_str = "EXAMPLE";
        let tag = MonitorTag::from(example_str.to_string());
        assert_eq!(format!("{:?}", tag), "MonitorTag(EXAMPLE)");
    }

    #[test]
    fn monitor_tag_score_debug() {
        let example_str = "EXAMPLE";
        let tag = MonitorTag::from(example_str.to_string());
        let mut writer = StringWriter::new();
        assert!(score_write!(&mut writer, "{:?}", tag).is_ok());
        assert_eq!(writer.get(), "MonitorTag(EXAMPLE)");
    }

    #[test]
    fn monitor_tag_from_string() {
        let example_str = "EXAMPLE";
        let tag = MonitorTag::from(example_str.to_string());
        compare_tag(tag.0, example_str);
    }

    #[test]
    fn monitor_tag_from_str() {
        let example_str = "EXAMPLE";
        let tag = MonitorTag::from(example_str);
        compare_tag(tag.0, example_str);
    }

    #[test]
    fn deadline_tag_debug() {
        let example_str = "EXAMPLE";
        let tag = DeadlineTag::from(example_str.to_string());
        assert_eq!(format!("{:?}", tag), "DeadlineTag(EXAMPLE)");
    }

    #[test]
    fn deadline_tag_score_debug() {
        let example_str = "EXAMPLE";
        let tag = DeadlineTag::from(example_str.to_string());
        let mut writer = StringWriter::new();
        assert!(score_write!(&mut writer, "{:?}", tag).is_ok());
        assert_eq!(writer.get(), "DeadlineTag(EXAMPLE)");
    }

    #[test]
    fn deadline_tag_from_string() {
        let example_str = "EXAMPLE";
        let tag = DeadlineTag::from(example_str.to_string());
        compare_tag(tag.0, example_str);
    }

    #[test]
    fn deadline_tag_from_str() {
        let example_str = "EXAMPLE";
        let tag = DeadlineTag::from(example_str);
        compare_tag(tag.0, example_str);
    }
}
