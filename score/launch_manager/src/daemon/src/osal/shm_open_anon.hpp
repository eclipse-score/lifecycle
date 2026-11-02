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

#ifndef OSAL_SHM_OPEN_ANON_HPP_INCLUDED
#define OSAL_SHM_OPEN_ANON_HPP_INCLUDED

namespace score::mw::lifecycle::internal::osal
{

/// @brief Open an anonymous shared memory file with close-on-exec behavior.
/// @param name to describe the shared memory. Does not affect the behavior of the file descriptor, and as such multiple
/// files can have the same name without any side effects.
/// @return On success, returns a new file descriptor. On error, -1 is returned and errno is set to indicate the error.
int shm_open_anon(const char* name);

}  // namespace score::mw::lifecycle::internal::osal

#endif
