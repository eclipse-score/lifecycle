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

#include <sys/unistd.h>

#include <score/lcm/internal/osal/osalnumcores.hpp>

namespace score {
namespace lcm {
namespace internal {
namespace osal {
uint32_t getNumCores() {
    long res = sysconf(_SC_NPROCESSORS_ONLN);
    if (res < 1 || res > static_cast<long>(kDefaultNumCores)) {
        return kDefaultNumCores;
    } else {
        return static_cast<uint32_t>(res);
    }
}
}  // namespace osal
}  // namespace lcm
}  // namespace internal
}  // namespace score
