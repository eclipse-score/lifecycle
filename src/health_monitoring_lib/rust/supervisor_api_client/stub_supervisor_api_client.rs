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

use crate::log::warn;
use crate::supervisor_api_client::SupervisorAPIClient;

/// A stub implementation of the SupervisorAPIClient that logs alive notifications.
#[allow(dead_code)]
pub struct StubSupervisorAPIClient;

impl StubSupervisorAPIClient {
    pub fn new() -> Self {
        Self
    }
}

#[allow(dead_code)]
impl SupervisorAPIClient for StubSupervisorAPIClient {
    fn notify_alive(&self) {
        warn!("StubSupervisorAPIClient: notify_alive called");
    }
}
