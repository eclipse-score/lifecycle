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

#ifndef ISUPERVISION_HPP_INCLUDED
#define ISUPERVISION_HPP_INCLUDED

#include <cstdint>

#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace score::mw::lifecycle::internal::saf::supervision
{

/// @brief ISupervision
/// @details The Interface Supervision class declares/defines methods, which are common for the different
/// supervision types which are: alive-, deadline-, logical-
class ISupervision
{
  public:
    /// @brief No default constructor
    ISupervision() = delete;

    /// @brief Constructor
    /// @param [in] f_supervisionConfigName_p       Unique hashed name set by configuration
    explicit ISupervision(const IdentifierHash f_supervisionConfigName_p) noexcept(true);

    /// @brief Default destructor
    virtual ~ISupervision() = default;

    /// @brief Trigger evaluation
    /// @details Cyclic evaluation trigger for the supervision.
    /// This method tells the supervision that all supervision interfaces were queried for new data
    /// and the collected data (checkpoints) is now ready for evaluation.
    /// @param [in] f_syncTimestamp   Timestamp for cyclic synchronization
    virtual void evaluate(const std::chrono::nanoseconds f_syncTimestamp) = 0;

    /// @brief Get the name of the configuration element for the corresponding supervision container
    /// @return The hashed name of the corresponding supervision configuration container
    IdentifierHash getConfigName(void) const noexcept;

  protected:
    /// @brief Default Move Constructor
    ISupervision(ISupervision&&) = default;
    /// @brief No Copy Constructor
    ISupervision(ISupervision&) = delete;
    /// @brief No Move assignment
    ISupervision& operator=(const ISupervision&&) = delete;
    /// @brief No Copy Assignment
    ISupervision& operator=(const ISupervision&) = delete;

  private:
    /// Unique name set by configuration
    const IdentifierHash k_cfgName;
};

}  // namespace score::mw::lifecycle::internal::saf::supervision

#endif
