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

#ifndef WATCHDOG_HPP_INCLUDED
#define WATCHDOG_HPP_INCLUDED

#ifndef __QNXNTO__
#include <linux/watchdog.h>
#else
// for _IOW, _IOR, _IOWR
#include <sys/ioctl.h>
#include <cstdint>
// Options for watchdog device interaction with ioctl.
// For QNX, these constants are not defined in a dedicated header file so we need to define them manually.
// For Linux, these constants are defined in linux/watchdog.h - we use the same naming here.
// Note that there are slight differences in the datatype of these constants for QNX compared to linux.
constexpr char WATCHDOG_IOCTL_BASE{'W'};
constexpr std::int32_t WDIOS_ENABLECARD{0x0002};
constexpr std::int32_t WDIOS_DISABLECARD{0x0001};

constexpr std::int32_t WDIOC_SETOPTIONS{_IOW(WATCHDOG_IOCTL_BASE, 4, std::int32_t)};

constexpr std::int32_t WDIOC_KEEPALIVE{_IOR(WATCHDOG_IOCTL_BASE, 5, std::int32_t)};

constexpr std::int32_t WDIOC_SETTIMEOUT{_IOWR(WATCHDOG_IOCTL_BASE, 6, std::int32_t)};

constexpr std::int32_t WDIOC_GETTIMEOUT{_IOR(WATCHDOG_IOCTL_BASE, 7, std::int32_t)};

constexpr std::int32_t WDIOC_GETTIMELEFT{_IOR(WATCHDOG_IOCTL_BASE, 10, std::int32_t)};
#endif

#endif
