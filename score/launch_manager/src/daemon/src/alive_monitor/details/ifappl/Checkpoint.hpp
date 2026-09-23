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

#ifndef CHECKPOINT_HPP_INCLUDED
#define CHECKPOINT_HPP_INCLUDED

#include <chrono>

namespace score::mw::lifecycle::internal::saf::ifappl
{

/// @brief Snapshot of a process at a point in time
struct Checkpoint
{
    /// @brief Data loss event marker
    bool isDataLossEvent;

    /// @brief Timestamp value in [nano seconds]
    std::chrono::nanoseconds timestamp;
};

}  // namespace score::mw::lifecycle::internal::saf::ifappl

#endif
