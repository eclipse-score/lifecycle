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
#include <score/lcm/processstatenotifier.hpp>
#include <score/lcm/processstatereceiver.hpp>

namespace score {
namespace lcm {
namespace internal {

ProcessStateNotifier::ProcessStateNotifier() noexcept {
    ring_buffer_ = std::make_shared<ipc_dropin::RingBuffer<
        static_cast<size_t>(score::lcm::BufferConstants::BUFFER_QUEUE_SIZE),
        static_cast<size_t>(score::lcm::BufferConstants::BUFFER_MAXPAYLOAD)>>();

    ring_buffer_->initialize();
}

ProcessStateNotifier::~ProcessStateNotifier() noexcept {
}

bool ProcessStateNotifier::queuePosixProcess(const score::lcm::PosixProcess& f_posixProcess) noexcept {
    bool ret = true;
    if (ring_buffer_->tryEnqueue(f_posixProcess)) {
        // nothing
    } else {
        LM_LOG_ERROR() << "Failed to queue posix process";
        ret = false;
    }
    return ret;
}

std::unique_ptr<score::lcm::IProcessStateReceiver> ProcessStateNotifier::constructReceiver() noexcept {
    return std::make_unique<score::lcm::ProcessStateReceiver>(ring_buffer_);
}

}  // namespace lcm
}  // namespace internal
}  // namespace score
