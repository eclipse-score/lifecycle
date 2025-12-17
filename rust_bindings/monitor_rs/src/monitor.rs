/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License Version 2.0 which is available at
* https://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/

use crate::errors;
use libc::{c_char, c_uint, c_void};
use std::ffi::CString;
use std::marker::PhantomData;

#[link(name = "lifecycle_client")]
unsafe extern "C" {
    fn score_lcm_monitor_initialize(instanceSpecifier: *const c_char) -> *mut c_void;
    fn score_lcm_monitor_deinitialize(instance: *mut c_void);
    fn score_lcm_monitor_report_checkpoint(instance: *mut c_void, checkpoint_id: c_uint);
}

pub struct Monitor<EnumT> {
    instance_ptr: *mut c_void,
    name: CString,
    phantom: PhantomData<EnumT>,
}

impl<EnumT> Monitor<EnumT> {
    pub fn new(instance: &str) -> Result<Self, Box<dyn std::error::Error>> {
        let tmp_str = CString::new(instance)?;
        let mut tmp_inst = Self {
            instance_ptr: std::ptr::null_mut(),
            name: tmp_str,
            phantom: PhantomData,
        };

        let ptr: *mut c_void;
        unsafe {
            ptr = score_lcm_monitor_initialize(tmp_inst.name.as_ptr());
        }

        if ptr.is_null() {
            return Err(Box::new(errors::ConstructorError {}));
        }
        tmp_inst.instance_ptr = ptr;

        Ok(tmp_inst)
    }

    pub fn report_checkpoint(&self, checkpoint_id: EnumT)
    where
        EnumT: Into<u32> + Copy,
    {
        let id: u32 = checkpoint_id.into();
        unsafe {
            score_lcm_monitor_report_checkpoint(self.instance_ptr, id);
        }
    }
}

impl<EnumT> Drop for Monitor<EnumT> {
    fn drop(&mut self) {
        unsafe {
            score_lcm_monitor_deinitialize(self.instance_ptr);
        }
    }
}
