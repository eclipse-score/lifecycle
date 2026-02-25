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

//! Module providing [`SupervisorAPIClient`] implementations.
//! Currently `ScoreSupervisorAPIClient` and `StubSupervisorAPIClient` are supported.
//! The latter is meant for testing purposes.

/// An abstraction over the API used to notify the supervisor about process liveness.
pub trait SupervisorAPIClient {
    fn notify_alive(&self);
}

// NOTE: various implementations are not mutually exclusive.

#[cfg(feature = "score_supervisor_api_client")]
pub mod score_supervisor_api_client;
#[cfg(feature = "stub_supervisor_api_client")]
pub mod stub_supervisor_api_client;
