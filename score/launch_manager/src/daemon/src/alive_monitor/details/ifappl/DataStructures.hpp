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

#ifndef DATASTRUCTURES_HPP_INCLUDED
#define DATASTRUCTURES_HPP_INCLUDED

#include <cstdint>

#include "score/mw/launch_manager/alive_monitor/details/ipc/IpcServer.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"

namespace score::mw::lifecycle::internal::saf::ifappl
{

/// @brief Maximum number of Checkpoints to be stored in IPC channel
/// @todo Implement logic to determine the number of checkpoint entries
/// that an Alive instance can report between two cycles
/// of AliveMonitor.
constexpr uint16_t k_maxCheckpointBufferElements{512U};

/// @brief Variable data exchange buffer: For every report of checkpoint,
/// one new instance of the below structure is created and stored
/// in the shared memory
struct CheckpointBufferElement final
{
    /// @brief Timestamp of the checkpoint
    std::chrono::nanoseconds timestamp{0U};

    /// @brief Default constructor needed for storage in vector
    CheckpointBufferElement() = default;

    /// @brief Constructor for usage with emplace
    /// @param [in] f_timestamp The checkpoint timestamp
    CheckpointBufferElement(std::chrono::nanoseconds f_timestamp) noexcept(true) : timestamp(f_timestamp)
    {
    }
};

/// @brief IPC server type instantiation with maximum checkpoint buffer size
using CheckpointIpcServer = ipc::IpcServer<CheckpointBufferElement, k_maxCheckpointBufferElements>;

}  // namespace score::mw::lifecycle::internal::saf::ifappl

#endif
