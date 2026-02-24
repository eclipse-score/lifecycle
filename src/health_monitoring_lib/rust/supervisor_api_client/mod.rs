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

//! Module for selecting [`SupervisorAPIClient`] implementation.
//! Currently `ScoreSupervisorAPIClient` and `StubSupervisorAPIClient` are supported.
//! The latter is meant for testing purposes.

/// An abstraction over the API used to notify the supervisor about process liveness.
pub trait SupervisorAPIClient {
    fn notify_alive(&self);
}

// Disallow both and none features.
#[cfg(any(
    all(feature = "score_supervisor_api_client", feature = "stub_supervisor_api_client"),
    not(any(feature = "score_supervisor_api_client", feature = "stub_supervisor_api_client"))
))]
compile_error!("Either 'score_supervisor_api_client' or 'stub_supervisor_api_client' must be enabled!");

#[cfg(feature = "score_supervisor_api_client")]
mod score_supervisor_api_client;
#[cfg(feature = "stub_supervisor_api_client")]
mod stub_supervisor_api_client;

#[cfg(feature = "score_supervisor_api_client")]
pub use score_supervisor_api_client::ScoreSupervisorAPIClient as SupervisorAPIClientImpl;
#[cfg(feature = "stub_supervisor_api_client")]
pub use stub_supervisor_api_client::StubSupervisorAPIClient as SupervisorAPIClientImpl;
