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

#ifndef SCORE_MW_LAUNCH_MANAGER_WATCHDOG_IWATCHDOG_HPP_INCLUDED
#define SCORE_MW_LAUNCH_MANAGER_WATCHDOG_IWATCHDOG_HPP_INCLUDED

namespace score::mw::lifecycle::internal::watchdog
{

/// @brief Interface between the Launch Manager and a watchdog.
/// @details The implementation is selected at build time with the `//config:watchdog_impl` label flag, so that
/// integrators can connect the Launch Manager to their own watchdog (hardware device, microcontroller, hypervisor,
/// ...).
///
/// Call sequence used by the Launch Manager:
/// 1. createWatchdog() (see watchdog_factory.hpp).
/// 2. init(), once, at the end of the Launch Manager initialization. Aftewards, a periodic call to serviceWatchdog() is
/// expected.
/// 3. serviceWatchdog(), once per main loop iteration.
/// 4. fireWatchdog() on a critical error. serviceWatchdog() is not called anymore afterwards.
/// 5. deinit() during shutdown of the Launch Manager.
///
/// All methods are called from the Launch Manager main thread.
class IWatchdog
{
  public:
    /// @brief Destructor.
    virtual ~IWatchdog() noexcept = default;

    /// @brief Initiate the watchdog.
    /// @return true on success. On false the Launch Manager aborts its startup.
    virtual bool init() noexcept = 0;

    /// @brief Services the watchdog.
    virtual void serviceWatchdog() noexcept = 0;

    /// @brief Reacts to a critical error of the Launch Manager.
    /// @details The Launch Manager must not continue normal operation after this call. The implementation triggers
    /// the system reaction, e.g. a reset or terminating the Launch Manager (the default implementation exits with
    /// EXIT_FAILURE).
    virtual void fireWatchdog() noexcept = 0;

    /// @brief Called during orderly shutdown of the Launch Manager.
    /// @details The implementation decides whether to disarm the watchdog, to keep it armed (to force a reset), or to
    /// do nothing. Must be safe to call even if init() was never called or failed.
    virtual void deinit() noexcept = 0;

  protected:
    /// @brief Default constructor.
    IWatchdog() noexcept = default;

    /// @brief No copy constructor.
    IWatchdog(const IWatchdog&) = delete;

    /// @brief No move constructor.
    IWatchdog(IWatchdog&&) = delete;

    /// @brief No copy assignment operator.
    IWatchdog& operator=(const IWatchdog&) = delete;

    /// @brief No move assignment operator.
    IWatchdog& operator=(IWatchdog&&) = delete;
};

}  // namespace score::mw::lifecycle::internal::watchdog

#endif  // SCORE_MW_LAUNCH_MANAGER_WATCHDOG_IWATCHDOG_HPP_INCLUDED
