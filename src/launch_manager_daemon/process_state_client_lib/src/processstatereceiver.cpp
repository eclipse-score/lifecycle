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

#include "score/lcm/processstatereceiver.hpp"
#include "score/lcm/internal/log.hpp"

namespace score {

namespace lcm {
ProcessStateReceiver::ProcessStateReceiver(BufferP ring_buffer) noexcept : ring_buffer_(ring_buffer) {
}

ProcessStateReceiver::~ProcessStateReceiver() noexcept {
}

score::Result<std::optional<PosixProcess>>
ProcessStateReceiver::getNextChangedPosixProcess() noexcept {
  score::lcm::PosixProcess changedProcess;
  if (ring_buffer_->getOverflowFlag()) {
    LM_LOG_ERROR()
        << "ProcessStateReceiver::getNextChangedPosixProcess: Overflow occurred, "
           "will be reported as kCommunicationError";
    return score::Result<std::optional<score::lcm::PosixProcess>>{
        score::MakeUnexpected(score::lcm::ExecErrc::kCommunicationError)};
  }

  if (ring_buffer_->empty()) {
    return score::Result<std::optional<score::lcm::PosixProcess>>{std::nullopt};
  }

  auto res = ring_buffer_->tryDequeue(changedProcess);
  if (res) {
    return score::Result<std::optional<score::lcm::PosixProcess>>{changedProcess};
  } else {
    return score::Result<std::optional<score::lcm::PosixProcess>>{score::MakeUnexpected(score::lcm::ExecErrc::kGeneralError)};
  }
}
} // namespace lcm
} // namespace score
