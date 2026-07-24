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
#include "score/mw/launch_manager/recovery_client/recovery_client.hpp"
#include <optional>

namespace score
{
namespace lcm
{

RecoveryClient::RecoveryClient() noexcept : ringBuffer_{}
{
    ringBuffer_.initialize();
}

bool RecoveryClient::sendRecoveryRequest(const score::lcm::IdentifierHash& process_identifier) noexcept
{
    if (!ringBuffer_.tryEnqueue(process_identifier))
    {
        overflow_flag_ = true;
        return false;
    }
    return true;
}

bool RecoveryClient::hasOverflow() const noexcept
{
    return overflow_flag_.load();
}

std::optional<score::lcm::IdentifierHash> RecoveryClient::getNextRequest() noexcept
{
    score::lcm::IdentifierHash req;
    if (ringBuffer_.tryDequeue(req))
    {
        return req;
    }
    return std::nullopt;
}
}  // namespace lcm
}  // namespace score
