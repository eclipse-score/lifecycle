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

use std::ffi::{CStr, c_char};
use crate::common::Tag;

#[unsafe(no_mangle)]
extern "C" fn hm_tag_from_str(name: *const c_char) -> Tag {
    let name = unsafe { CStr::from_ptr(name) };

    match name.to_str() {
        Ok(name) => Tag::from_str(name),
        Err(_err) => todo!("Report error"),
    }
}
