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

#ifndef DEVICEIFMOCK_HPP_INCLUDED
#define DEVICEIFMOCK_HPP_INCLUDED

#include "score/mw/launch_manager/watchdog/details/DeviceIf.hpp"

#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal::watchdog
{

/// @brief Reusable gmock mock for DeviceIf, for use by tests of components that issue ioctl calls on device files.
class MockDeviceIf : public DeviceIf
{
  public:
    MOCK_METHOD(
        std::int32_t,
        ioctl,
        (std::int32_t f_fd, IoctlRequestType f_request, std::int32_t* f_payload_p),
        (noexcept, override));
};

}  // namespace score::mw::lifecycle::internal::watchdog

#endif  // DEVICEIFMOCK_HPP_INCLUDED
