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
#include <score/lcm/internal/recovery_client.hpp>

namespace score {
namespace lcm {

inline score::concurrency::InterruptibleFuture<void> GetErrorFuture(score::lcm::ExecErrc errType) noexcept
{
    score::concurrency::InterruptiblePromise<void> tmp_ {};
    tmp_.SetError( errType );
    return tmp_.GetInterruptibleFuture().value();
}

RecoveryClient::RecoveryClient() noexcept  : ringBuffer_{}, requests_{} {
    ringBuffer_.initialize();
}

score::concurrency::InterruptibleFuture<void> RecoveryClient::sendRecoveryRequest(
            const score::lcm::IdentifierHash& pg_name, const score::lcm::IdentifierHash& pg_state) noexcept {
    std::size_t i = 0u;
    score::concurrency::InterruptibleFuture<void> retVal;
    for (; i < requests_.size(); ++i) {
        bool expected = false;
        if (requests_[i].in_use_.compare_exchange_strong(expected, true)) {
            break;
        }
    }

    if (i < requests_.size()) {
        requests_[i].promise_ = score::concurrency::InterruptiblePromise<void>{};
        auto futureResult = requests_[i].promise_.GetInterruptibleFuture();
        if (futureResult.has_value()) {
            retVal = std::move(futureResult.value());
        } else {
            requests_[i].in_use_ = false;
            return GetErrorFuture(ExecErrc::kFailed);
        }

        RecoveryRequest req{pg_name, pg_state, i};
        if(!ringBuffer_.tryEnqueue(req)) {
            requests_[i].promise_.SetError(score::lcm::ExecErrc::kFailed);
            requests_[i].in_use_ = false;
        }
    } else {
        retVal = GetErrorFuture(ExecErrc::kFailed);
    }
    return retVal;
}

void RecoveryClient::setResponseSuccess(std::size_t promise_id) noexcept {
    requests_[promise_id].promise_.SetValue();
    requests_[promise_id].in_use_ = false;
}

void RecoveryClient::setResponseError(std::size_t promise_id, score::lcm::ExecErrc errType) noexcept {
    requests_[promise_id].promise_.SetError(errType);
    requests_[promise_id].in_use_ = false;
}

RecoveryRequest* RecoveryClient::getNextRequest() noexcept {
    if(ringBuffer_.tryDequeue(temp_request_)) {
        return &temp_request_;
    } else {
        return nullptr;
    }
}
}
}