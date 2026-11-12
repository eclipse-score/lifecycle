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

#include <sys/mman.h>

#include "score/mw/launch_manager/osal/shm_open_anon.hpp"

namespace score::mw::lifecycle::internal::osal
{

int shm_open_anon(const char* name)
{
    return memfd_create(name, MFD_CLOEXEC);
}

}  // namespace score::mw::lifecycle::internal::osal
