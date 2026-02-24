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

use crate::deadline::DeadlineEvaluationError;
use crate::log::ScoreDebug;
use crate::tag::MonitorTag;
use core::hash::Hash;
use core::time::Duration;
use std::sync::Arc;

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

/// The monitor has an evaluation handle available.
pub(crate) trait HasEvalHandle {
    /// Get an evaluation handle for this monitor.
    ///
    /// # NOTE
    ///
    /// This method is intended to be called from a background thread periodically.
    fn get_eval_handle(&self) -> MonitorEvalHandle;
}

/// Errors that can occur during monitor evaluation.
/// Contains failing monitor type.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, ScoreDebug)]
#[allow(dead_code)]
pub(crate) enum MonitorEvaluationError {
    Deadline(DeadlineEvaluationError),
    Heartbeat,
    Logic,
}

impl From<DeadlineEvaluationError> for MonitorEvaluationError {
    fn from(value: DeadlineEvaluationError) -> Self {
        MonitorEvaluationError::Deadline(value)
    }
}

/// Trait for evaluating monitors and reporting errors to be used by HealthMonitor.
pub(crate) trait MonitorEvaluator {
    /// Run monitor evaluation.
    ///
    /// - `on_error` - error handling, containing tag of failing object and error code.
    fn evaluate(&self, on_error: &mut dyn FnMut(&MonitorTag, MonitorEvaluationError));
}

/// Handle to a monitor evaluator, allowing for dynamic dispatch.
pub(crate) struct MonitorEvalHandle {
    inner: Arc<dyn MonitorEvaluator + Send + Sync>,
}

impl MonitorEvalHandle {
    pub(crate) fn new<T: MonitorEvaluator + Send + Sync + 'static>(inner: Arc<T>) -> Self {
        Self { inner }
    }
}

impl MonitorEvaluator for MonitorEvalHandle {
    fn evaluate(&self, on_error: &mut dyn FnMut(&MonitorTag, MonitorEvaluationError)) {
        self.inner.evaluate(on_error)
    }
}
