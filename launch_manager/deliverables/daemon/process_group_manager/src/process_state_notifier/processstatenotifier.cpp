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

#include <score/lcm/internal/log.hpp>
#include <score/lcm/internal/process_state_notifier/processstatenotifier.hpp>

namespace score {
namespace lcm {
namespace internal {

ProcessStateNotifier::ProcessStateNotifier() noexcept {
}

ProcessStateNotifier::~ProcessStateNotifier() noexcept {
    static_cast<void>(m_LCM_PHM_socket.close());
}

bool ProcessStateNotifier::init() noexcept {
    bool ret = true;
    /* RULECHECKER_comment(1, 1, check_octal_constant, "The API takes as a parameter an octal constant", true); */
    if (m_LCM_PHM_socket.create("ProcessState", 0260U) != ipc_dropin::ReturnCode::kOk) {
        LM_LOG_ERROR() << "Failed to create LCM-PHM socket";
        ret = false;
    } else {
        LM_LOG_DEBUG() << "ProcessStateNotifier::init successfully executed";
    }
    return ret;
}

bool ProcessStateNotifier::queuePosixProcess(const score::lcm::PosixProcess& f_posixProcess) noexcept {
    bool ret = true;
    if (m_LCM_PHM_socket.trySend(f_posixProcess) == ipc_dropin::ReturnCode::kOk) {
        // nothing
    } else {
        LM_LOG_ERROR() << "Failed to queue posix process";
        ret = false;
    }
    return ret;
}

}  // namespace lcm
}  // namespace internal
}  // namespace score
