/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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
#ifndef SCORE_HM_FFI_HELPERS_HPP
#define SCORE_HM_FFI_HELPERS_HPP

#include <score/hm/common.h>
#include <cassert>

namespace score::hm::ffi
{

inline Error fromRustError(int ffi_error_code)
{
    switch (ffi_error_code)
    {
        case 1:
            return Error::NotFound;
        case 2:
            return Error::AlreadyExists;
        case 3:
            return Error::InvalidArgument;
        case 4:
            return Error::WrongState;
        case 5:
            return Error::Failed;
        default:
            assert(false && "Unknown FFI error code");
            return Error::InvalidArgument;  // Fallback
    }
}

}  // namespace score::hm::ffi

#endif  // SCORE_HM_FFI_HELPERS_HPP
