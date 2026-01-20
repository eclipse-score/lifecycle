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
#include <sys/neutrino.h>

#include <score/lcm/internal/osal/setaffinity.hpp>
namespace score {

namespace lcm {

namespace internal {

namespace osal {

int32_t setaffinity(uint32_t cpumask) noexcept(true) {
    return 0 == ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET, &cpumask) ? 0 : -1;
}
}  // namespace osal
}  // namespace lcm
}  // namespace internal
}  // namespace score
