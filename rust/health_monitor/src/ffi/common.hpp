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

#ifndef HM_COMMON_H
#define HM_COMMON_H

#include <cstdint>
#include <chrono>

enum class hm_Status
{
    Running,
    Disabled,
    Failed,
};

enum class hm_Error
{
    NoError,
    BadParameter,
    DoesNotExist,
    NotAllowed,
    OutOfMemory,
    Generic,
};

struct hm_Tag {
    uint64_t hash;
};

extern "C"
{
    hm_Tag hm_tag_from_str(const char *name);
}

namespace hm
{

using Status = hm_Status;
using Error = hm_Error;

struct Tag: hm_Tag {
    Tag(const char *name) : hm_Tag(hm_tag_from_str(name)) {}
    Tag(hm_Tag &&ffi_tag) : hm_Tag(std::move(ffi_tag)) {}
};

using Duration = std::chrono::milliseconds;

struct DurationRange {
    Duration min;
    Duration max;
};

}

#endif //HM_COMMON_H
