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
#ifndef SCORE_LCM_IRECOVERYCLIENT_H_
#define SCORE_LCM_IRECOVERYCLIENT_H_

#include <optional>
#include <score/lcm/identifier_hash.hpp>

namespace score {
namespace lcm {

/// @brief Represents a recovery request for a failed process group.
struct RecoveryRequest {
    /// @brief The id of the process group the failed process is running in
    score::lcm::IdentifierHash process_group_identifier_{};
};

/// @brief The RecoveryClient allows the HealthMonitor component to report supervision failures to the ProcessGroupManager
/// thus requesting recovery for a specific process group.
/// The requests are queued and periodically processed by the ProcessGroupManager.
/// In case the buffer is full and request cannot be queued, the overflow flag is set.
/// A detected overflow shall be handled as a critical failure by the ProcessGroupManager.
class IRecoveryClient {
public:
    IRecoveryClient() noexcept = default;
    virtual ~IRecoveryClient() noexcept = default;
    IRecoveryClient(const IRecoveryClient&) = delete;
    IRecoveryClient& operator=(const IRecoveryClient&) = delete;
    IRecoveryClient(IRecoveryClient&&) = delete;
    IRecoveryClient& operator=(IRecoveryClient&&) = delete;

    /// @brief Send recovery request for a specific process group.
    /// @details If the internal buffer is full, the request is discarded and the overflow flag is set.
    /// @param process_group_identifier The process group that requires recovery.
    /// @return true if the request was queued, false if the buffer was full.
    virtual bool sendRecoveryRequest(const score::lcm::IdentifierHash& process_group_identifier) noexcept = 0;
    /// @brief Retrieve the latest request from the queue, removing it from the queue
    /// @return The request, or std::nullopt if no request is available
    virtual std::optional<RecoveryRequest> getNextRequest() noexcept = 0;

    /// @brief Checks if overflow has been set, by previously calling `sendRecoveryRequest` while the queue was already full
    /// @details Since overflow is a critical error, the flag is never reset
    /// @return True if overflow has occurred, else false.
    virtual bool hasOverflow() const noexcept = 0;
};
} // namespace lcm
} // namespace score

#endif
