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

#include "score/mw/launch_manager/watchdog/details/DeviceIf.hpp"

#include <sys/ioctl.h>

namespace score::mw::lifecycle::internal::watchdog
{

DeviceIf& DeviceIf::instance() noexcept
{
    static DeviceIf instance;
    return instance;
}

std::int32_t DeviceIf::ioctl(std::int32_t f_fd, IoctlRequestType f_request, std::int32_t* f_payload_p) noexcept
{
    // Wrapper over the real syscall.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg, score-banned-function)
    return ::ioctl(f_fd, f_request, f_payload_p);
}

}  // namespace score::mw::lifecycle::internal::watchdog
