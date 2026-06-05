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
use crate::errors;
use libc::{c_char, c_uint, c_void};
use std::ffi::CString;

#[link(name = "lifecycle_client")]
unsafe extern "C" {
    fn score_lcm_alive_initialize(instanceSpecifier: *const c_char) -> *mut c_void;
    fn score_lcm_alive_deinitialize(instance: *mut c_void);
    fn score_lcm_alive_report_checkpoint(instance: *mut c_void, checkpoint_id: c_uint);
}

pub struct Alive {
    instance_ptr: *mut c_void,
    name: CString,
}

impl Alive {
    pub fn new(instance: &str) -> Result<Self, Box<dyn std::error::Error>> {
        let tmp_str = CString::new(instance)?;
        let mut tmp_inst = Self {
            instance_ptr: std::ptr::null_mut(),
            name: tmp_str,
        };

        let ptr: *mut c_void;
        unsafe {
            ptr = score_lcm_alive_initialize(tmp_inst.name.as_ptr());
        }

        if ptr.is_null() {
            return Err(Box::new(errors::ConstructorError {}));
        }
        tmp_inst.instance_ptr = ptr;

        Ok(tmp_inst)
    }

    pub fn report_checkpoint(&self, checkpoint_id: u32) {
        let id: u32 = checkpoint_id.into();
        unsafe {
            score_lcm_alive_report_checkpoint(self.instance_ptr, id);
        }
    }
}

impl Drop for Alive {
    fn drop(&mut self) {
        unsafe {
            score_lcm_alive_deinitialize(self.instance_ptr);
        }
    }
}
