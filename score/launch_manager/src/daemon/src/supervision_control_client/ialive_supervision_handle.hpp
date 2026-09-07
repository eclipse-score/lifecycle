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
#ifndef IALIVE_SUPERVISION_HANDLE_HPP_INCLUDED
#define IALIVE_SUPERVISION_HANDLE_HPP_INCLUDED

#include <ctime>

namespace score::mw::lifecycle
{

/// @brief IAliveSupervisionHandle interface for requesting alive supervision changes.
class IAliveSupervisionHandle
{
  public:
    /// @brief Destructor.
    virtual ~IAliveSupervisionHandle() noexcept = default;

    /// @brief Request that the calling process begins supervision at @param time
    virtual bool activateSupervision(timespec time) noexcept = 0;

    /// @brief Request that the calling process stops supervision at @param time
    virtual bool deactivateSupervision(timespec time) noexcept = 0;
};

}  // namespace score::mw::lifecycle

#endif
