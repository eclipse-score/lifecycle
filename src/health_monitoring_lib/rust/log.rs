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

//! Logging module.
//! Utilizes `"HMON"` context by default.

#![allow(unused_macros)]

pub(crate) const CONTEXT: &str = "HMON";

/// Proxy for `score_log::fatal!`.
#[clippy::format_args]
macro_rules! fatal {
    ($($arg:tt)+) => (score_log::fatal!(context: $crate::log::CONTEXT, $($arg)+));
}

/// Proxy for `score_log::error!`.
#[clippy::format_args]
macro_rules! error {
    ($($arg:tt)+) => (score_log::error!(context: $crate::log::CONTEXT, $($arg)+));
}

/// Proxy for `score_log::warn!`.
#[clippy::format_args]
macro_rules! warning {
    ($($arg:tt)+) => (score_log::warn!(context: $crate::log::CONTEXT, $($arg)+));
}

/// Proxy for `score_log::info!`.
#[clippy::format_args]
macro_rules! info {
    ($($arg:tt)+) => (score_log::info!(context: $crate::log::CONTEXT, $($arg)+));
}

/// Proxy for `score_log::debug!`.
#[clippy::format_args]
macro_rules! debug {
    ($($arg:tt)+) => (score_log::debug!(context: $crate::log::CONTEXT, $($arg)+));
}

/// Proxy for `score_log::trace!`.
#[clippy::format_args]
macro_rules! trace {
    ($($arg:tt)+) => (score_log::trace!(context: $crate::log::CONTEXT, $($arg)+));
}

// Export macros from this module (e.g., `crate::log::error`).
// `#[macro_export]` would export them from crate (e.g., `crate::error`).
//
// `warning as warn` is due to `warn` macro name conflicting with `warn` attribute.
#[allow(unused_imports)]
pub(crate) use {debug, error, fatal, info, trace, warning as warn};

// Re-export symbols from `score_log`.
pub(crate) use score_log::fmt::{score_write, DebugStruct, Error, FormatSpec, ScoreDebug, Writer};
pub(crate) use score_log::ScoreDebug;
