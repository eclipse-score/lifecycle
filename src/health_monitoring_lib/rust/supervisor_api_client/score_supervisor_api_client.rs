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

use crate::log::debug;
use crate::supervisor_api_client::SupervisorAPIClient;
use crate::worker::Checks;

#[allow(dead_code)]
pub struct ScoreSupervisorAPIClient {
    supervisor_link: monitor_rs::Monitor<Checks>,
}

unsafe impl Send for ScoreSupervisorAPIClient {} // Just assuming it's safe to send across threads, this is a temporary solution

#[allow(dead_code)]
impl ScoreSupervisorAPIClient {
    pub fn new() -> Self {
        let value = std::env::var("IDENTIFIER").expect("IDENTIFIER env not set");
        debug!("ScoreSupervisorAPIClient: Creating with IDENTIFIER={}", value);
        // This is only temporary usage so unwrap is fine here.
        let supervisor_link = monitor_rs::Monitor::<Checks>::new(&value).expect("Failed to create supervisor_link");
        Self { supervisor_link }
    }
}

impl SupervisorAPIClient for ScoreSupervisorAPIClient {
    fn notify_alive(&self) {
        self.supervisor_link.report_checkpoint(Checks::WorkerCheckpoint);
    }
}
