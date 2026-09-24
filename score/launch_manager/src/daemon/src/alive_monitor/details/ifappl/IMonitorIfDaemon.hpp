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

#ifndef IMonitorIfDaemon_HPP_INCLUDED
#define IMonitorIfDaemon_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/details/common/Observer.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"

namespace score::mw::lifecycle::internal::saf::ifappl
{

/// @brief Reads checkpoints from IPC channel and pushes them to attached observers
class IMonitorIfDaemon : public common::Observer<ifexm::ObservableEvent>, public common::Observable<Checkpoint>
{
  public:
    /// @brief Constructor
    IMonitorIfDaemon() = default;

    /// @brief Destructor
    virtual ~IMonitorIfDaemon() = default;

    /// @brief No Copy Constructor
    IMonitorIfDaemon(const IMonitorIfDaemon&) = delete;
    /// @brief No Copy Assignment
    IMonitorIfDaemon& operator=(const IMonitorIfDaemon&) = delete;
    /// @brief No Move Assignment
    IMonitorIfDaemon& operator=(IMonitorIfDaemon&&) = delete;

    IMonitorIfDaemon(IMonitorIfDaemon&&) = default;

    /// @brief Check for new data
    /// @details Check Alive interface for new data from application side
    /// @param [in]  f_syncTimestamp    Timestamp till data shall be read, newer data will not be considered
    virtual void checkForNewData(const std::chrono::nanoseconds f_syncTimestamp) noexcept(true) = 0;
};

}  // namespace score::mw::lifecycle::internal::saf::ifappl

#endif
