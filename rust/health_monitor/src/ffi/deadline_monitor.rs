// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0

use std::ptr::null_mut;
use crate::{common::*, deadline_monitor::*};

//
// DeadlineMonitorBuilder
//

#[unsafe(no_mangle)]
extern "C" fn hm_dmb_new() -> *mut DeadlineMonitorBuilder {
    Box::into_raw(Box::new(DeadlineMonitorBuilder::new()))
}

#[unsafe(no_mangle)]
extern "C" fn hm_dmb_delete(builder_ptr: *mut *mut DeadlineMonitorBuilder) {
    let _builder = unsafe { Box::from_raw(*builder_ptr) };
    unsafe {
        *builder_ptr = null_mut();
    }
}

#[unsafe(no_mangle)]
extern "C" fn hm_dmb_add_deadline(builder: *mut DeadlineMonitorBuilder, tag: Tag, min_ms: u64, max_ms: u64) {
    unsafe { (*builder).add_deadline(tag, DurationRange::from_millis(min_ms, max_ms)) };
}

#[unsafe(no_mangle)]
extern "C" fn hm_dmb_build(builder_ptr: *mut *mut DeadlineMonitorBuilder, out: *mut *mut DeadlineMonitor) -> Error {
    let builder = unsafe { Box::from_raw(*builder_ptr) };
    unsafe {
        *builder_ptr = null_mut();
    }

    match builder.build() {
        Ok(monitor) => {
            unsafe {
                *out = Box::into_raw(Box::new(monitor));
            }

            Error::_NoError
        },
        Err(err) => err
    }
}

//
// DeadlineMonitor
//

#[unsafe(no_mangle)]
extern "C" fn hm_dm_delete(monitor: *mut *mut DeadlineMonitor) {
    let _monitor = unsafe { Box::from_raw(*monitor) };
    unsafe {
        *monitor = null_mut();
    }
}

#[unsafe(no_mangle)]
extern "C" fn hm_dm_get_deadline(monitor: *mut DeadlineMonitor, tag: Tag, out: *mut *mut Deadline) -> Error {
    let result = unsafe {
        (*monitor).get_deadline(tag)
    };

    match result {
        Ok(deadline) => {
            unsafe {
                *out = Box::into_raw(Box::new(deadline));
            }

            Error::_NoError
        }
        Err(err) => err,
    }
}

#[unsafe(no_mangle)]
extern "C" fn hm_dm_create_custom_deadline(monitor: *mut DeadlineMonitor, min_ms: u64, max_ms: u64, out: *mut *mut Deadline) -> Error {
    let result = unsafe {
        (*monitor).create_custom_deadline(DurationRange::from_millis(min_ms, max_ms))
    };

    match result {
        Ok(deadline) => {
            unsafe {
                *out = Box::into_raw(Box::new(deadline));
            }

            Error::_NoError
        },
        Err(err) => err,
    }
}

#[unsafe(no_mangle)]
extern "C" fn hm_dm_enable(monitor: *mut DeadlineMonitor) -> Error {
    match unsafe { (*monitor).enable() } {
        Ok(_) => Error::_NoError,
        Err(err) => err
    }
}

#[unsafe(no_mangle)]
extern "C" fn hm_dm_disable(monitor: *mut DeadlineMonitor) -> Error {
    match unsafe { (*monitor).disable() } {
        Ok(_) => Error::_NoError,
        Err(err) => err
    }
}

#[unsafe(no_mangle)]
extern "C" fn hm_dm_status(monitor: *const DeadlineMonitor) -> Status {
    unsafe { (*monitor).status() }
}

//
// Deadline
//

#[unsafe(no_mangle)]
extern "C" fn hm_dl_delete(deadline: *mut *mut Deadline) {
    let _deadline = unsafe { Box::from_raw(*deadline) };
    unsafe {
        *deadline = null_mut();
    }
}

#[unsafe(no_mangle)]
extern "C" fn hm_dl_start(deadline: *mut Deadline) -> Error {
     match unsafe { (*deadline).start() } {
        Ok(_) => Error::_NoError,
        Err(err) => err
    }
}

#[unsafe(no_mangle)]
extern "C" fn hm_dl_stop(deadline: *mut Deadline) -> Error {
    match unsafe { (*deadline).stop() } {
        Ok(_) => Error::_NoError,
        Err(err) => err
    }
}

#[unsafe(no_mangle)]
extern "C" fn hm_dl_min_ms(deadline: *const Deadline) -> u64 {
    unsafe { (*deadline).range().min.as_millis() as u64 }
}

#[unsafe(no_mangle)]
extern "C" fn hm_dl_max_ms(deadline: *const Deadline) -> u64 {
    unsafe { (*deadline).range().max.as_millis() as u64 }
}
