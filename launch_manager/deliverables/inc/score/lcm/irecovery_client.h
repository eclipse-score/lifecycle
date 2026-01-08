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

#include "score/concurrency/future/interruptible_future.h"
#include "score/concurrency/future/interruptible_promise.h"
#include <score/lcm/exec_error_domain.h>
#include <score/lcm/identifier_hash.hpp>

namespace score {
namespace lcm {

struct RecoveryRequest {
    score::lcm::IdentifierHash pg_name_;
    score::lcm::IdentifierHash pg_state_name_;
    std::size_t promise_id_;
};

class IRecoveryClient {
public:
    IRecoveryClient() noexcept = default;
    ~IRecoveryClient() noexcept = default;
    IRecoveryClient(const IRecoveryClient&) = delete;
    IRecoveryClient& operator=(const IRecoveryClient&) = delete;
    IRecoveryClient(IRecoveryClient&&) = delete;
    IRecoveryClient& operator=(IRecoveryClient&&) = delete;

    virtual score::concurrency::InterruptibleFuture<void> sendRecoveryRequest(
        const score::lcm::IdentifierHash& pg_name, const score::lcm::IdentifierHash& pg_state) noexcept = 0;

    virtual void setResponseSuccess(std::size_t promise_id) noexcept = 0;
    virtual void setResponseError(std::size_t promise_id, score::lcm::ExecErrc errType) noexcept = 0;
    virtual RecoveryRequest* getNextRequest() noexcept = 0;
}; 
} // namespace lcm
} // namespace score

#endif