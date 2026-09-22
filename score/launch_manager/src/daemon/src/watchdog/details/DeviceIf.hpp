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

#ifndef DEVICEIF_HPP_INCLUDED
#define DEVICEIF_HPP_INCLUDED

#include <cstdint>

namespace score::mw::lifecycle::internal::watchdog
{
/// @brief Wrapper for syscalls used to access a POSIX device
class DeviceIf
{
  public:
#ifdef __QNXNTO__
    /// @brief Request type for ioctl command
    using IoctlRequestType = std::int32_t;
#else
    /// @brief Request type for ioctl command
    using IoctlRequestType = std::uint64_t;
#endif
    /// @brief Returns the production singleton instance.
    static DeviceIf& instance() noexcept;

    /// @brief Manipulate the underlying device
    /// @details For details refer to the official documentation https://man7.org/linux/man-pages/man2/ioctl.2.html
    /// @param[in] f_fd The file descriptor of the opened device file
    /// @param[in] f_request The code identifying the operation
    /// @param[in,out] f_payload_p The input or output of the operation
    /// @returns -1 on error, >=0 on success
    virtual std::int32_t ioctl(std::int32_t f_fd, IoctlRequestType f_request, std::int32_t* f_payload_p) noexcept;

    /// @brief Default destructor.
    virtual ~DeviceIf() = default;
};

}  // namespace score::mw::lifecycle::internal::watchdog

#endif
