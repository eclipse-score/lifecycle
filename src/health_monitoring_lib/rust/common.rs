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

/// Errors that can occur during monitor evaluation.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, crate::log::ScoreDebug)]
pub(crate) enum MonitorEvaluationError {
    TooEarly,
    TooLate,
}

/// Trait for evaluating monitors and reporting errors to be used by HealthMonitor.
pub(crate) trait MonitorEvaluator {
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
