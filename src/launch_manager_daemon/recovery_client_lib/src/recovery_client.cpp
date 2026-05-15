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
#include <optional>
#include <score/lcm/recovery_client.hpp>

namespace score {
namespace lcm {

RecoveryClient::RecoveryClient() noexcept : ringBuffer_{} {
    ringBuffer_.initialize();
}

bool RecoveryClient::sendRecoveryRequest(const score::lcm::IdentifierHash& process_group_identifier) noexcept {
    RecoveryRequest req{process_group_identifier};
    if (!ringBuffer_.tryEnqueue(req)) {
        overflow_flag_ = true;
        return false;
    }
    return true;
}

bool RecoveryClient::hasOverflow() const noexcept {
    return overflow_flag_.load();
}

std::optional<RecoveryRequest> RecoveryClient::getNextRequest() noexcept {
    RecoveryRequest req;
    if(ringBuffer_.tryDequeue(req)) {
        return req;
    }
    return std::nullopt;
}
}
}
