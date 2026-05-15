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
#ifndef SCORE_LCM_RECOVERYCLIENT_H_
#define SCORE_LCM_RECOVERYCLIENT_H_

#include <atomic>
#include <cstddef>

#include "ipc_dropin/ringbuffer.hpp"
#include "score/lcm/irecovery_client.h"

namespace score {
namespace lcm {

class RecoveryClient final : public IRecoveryClient {
public:
    /// @brief Number of requests that can be stored before overflow occurs
    static constexpr std::size_t kBufferCapacity = 128;

    RecoveryClient() noexcept;
    ~RecoveryClient() noexcept = default;
    RecoveryClient(const RecoveryClient&) = delete;
    RecoveryClient& operator=(const RecoveryClient&) = delete;
    RecoveryClient(RecoveryClient&&) = delete;
    RecoveryClient& operator=(RecoveryClient&&) = delete;

    bool sendRecoveryRequest(const score::lcm::IdentifierHash& process_group_identifier) noexcept override;
    std::optional<RecoveryRequest> getNextRequest() noexcept override;
    bool hasOverflow() const noexcept override;

private:
    static const std::size_t element_size_ = sizeof(RecoveryRequest);
    ipc_dropin::RingBuffer<RecoveryClient::kBufferCapacity, RecoveryClient::element_size_> ringBuffer_;  ///< Ring buffer to store recovery requests
    std::atomic_bool overflow_flag_{false};
};
} // namespace lcm
} // namespace score

#endif
