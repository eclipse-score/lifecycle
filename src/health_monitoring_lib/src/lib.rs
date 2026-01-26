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
mod common;
mod ffi;
mod log;
mod protected_memory;

use std::collections::HashMap;

pub mod deadline;
pub use common::{IdentTag, TimeRange};

#[derive(Default)]
pub struct HealthMonitorBuilder {
    deadlines: HashMap<IdentTag, deadline::DeadlineMonitorBuilder>,
}

impl HealthMonitorBuilder {
    pub fn new() -> Self {
        Self {
            deadlines: HashMap::new(),
        }
    }

    pub fn add_deadline_monitor(&mut self, tag: IdentTag, monitor: deadline::DeadlineMonitorBuilder) {
        self.deadlines.insert(tag, monitor);
    }

    pub fn build(self) -> HealthMonitor {
        let allocator = protected_memory::ProtectedMemoryAllocator {};
        let mut monitors = HashMap::new();
        for (tag, builder) in self.deadlines {
            monitors.insert(tag, builder.build(&allocator));
        }
        HealthMonitor {
            deadline_monitors: monitors,
        }
    }
}

pub struct HealthMonitor {
    deadline_monitors: HashMap<IdentTag, deadline::DeadlineMonitor>,
}

impl HealthMonitor {
    pub fn get_deadline_monitor(&mut self, tag: &IdentTag) -> Option<deadline::DeadlineMonitor> {
        self.deadline_monitors.remove(tag)
    }
}
