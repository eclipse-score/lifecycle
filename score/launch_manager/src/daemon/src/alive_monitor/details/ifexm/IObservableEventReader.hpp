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

#ifndef I_OBSERVABLEEVENTREADER_HPP_INCLUDED
#define I_OBSERVABLEEVENTREADER_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"

namespace score::mw::lifecycle::internal::saf::ifexm
{

/// @brief Observable Event reader interface
/// @details The Observable Event reader fetches supervision events via the lcm library and distributes
/// the information to the Observable Event classes.
class IObservableEventReader
{
  public:
    virtual ~IObservableEventReader() = default;

    /// @brief Register observable events for reader
    /// @param [in]  f_processState_r   Process state to be registered
    /// @param [in]  f_processId        Process ID
    /// @return     true (registered), false (not registered)
    virtual bool registerObservableEvent(ObservableEvent& f_processState_r, const IdentifierHash f_processId) noexcept(
        false) = 0;

    /// @brief Deregister observable events from reader
    /// @param [in]  f_processId        Process ID to deregister the particular process
    virtual void deregisterObservableEvent(const IdentifierHash f_processId) noexcept = 0;

    /// @brief Distribute changes
    /// @details Distribute supervision events to the registered Observable Event classes
    /// @param [in] f_syncTimestamp   Timestamp for cyclic synchronization
    /// @return     true (successful distribution), false (failed distribution)
    virtual bool distributeChanges(const std::chrono::nanoseconds f_syncTimestamp) noexcept = 0;
};

}  // namespace score::mw::lifecycle::internal::saf::ifexm

#endif
