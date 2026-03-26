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
#ifndef CONTROL_CLIENT_H_
#define CONTROL_CLIENT_H_

#include "score/concurrency/future/interruptible_future.h"
#include "score/concurrency/future/interruptible_promise.h"
#include "score/result/result.h"
#include <string_view>
#include <memory>
#include "score/lcm/exec_error_domain.h"

namespace score {

namespace lcm {

class ControlClientImpl;

/// @brief Class representing connection to Launch Manager that is used to request Run Target activation (or other operations).
/// @note ControlClient opens communication channel to Launch Manager (e.g. POSIX FIFO). Each Process that intends to perform state management, shall create an instance of this class and it shall have rights to use it.
///
class ControlClient final {
   public:
    /// @brief Constructor that creates Control Client instance.
    ///
    explicit ControlClient() noexcept;

    /// @brief Destructor of the Control Client instance.
    /// @param  None.
    ///
    ~ControlClient() noexcept;

    // Applying the rule of five
    // Class will not be copyable, but it will be movable

    /// @brief Suppress default copy construction for ControlClient.
    ControlClient(const ControlClient&) = delete;

    /// @brief Suppress default copy assignment for ControlClient.
    ControlClient& operator=(const ControlClient&) = delete;

    /// @brief Intentional use of default move constructor for ControlClient.
    ///
    /// @param[in] rval reference to move
    ControlClient(ControlClient&& rval) noexcept;

    /// @brief Intentional use of default move assignment for ControlClient.
    ///
    /// @param[in] rval reference to move
    /// @returns the new reference
    ControlClient& operator=(ControlClient&& rval) noexcept;

    /// @brief Method to request activation of a specific Run Target.
    ///
    /// This method will request Launch Manager to activate a Run Target and return immediately.
    /// Returned InterruptibleFuture can be used to determine result of the activation request.
    ///
    /// @param[in] runTargetName name of the Run Target that should be activated. Launch Manager will deactivate the currently active Run Target and activate Run Target identified by this parameter.
    /// @returns void if activation requested is successful, otherwise it returns ExecErrorDomain error.
    /// @error score::lcm::ExecErrc::kCancelled if activation requested was cancelled by a newer request
    /// @error score::lcm::ExecErrc::kFailed if activation requested failed
    /// @error score::lcm::ExecErrc::kFailedUnexpectedTerminationOnExit if Unexpected Termination of a Process assigned to the previously active Run Target happened.
    /// @error score::lcm::ExecErrc::kFailedUnexpectedTerminationOnEnter if Unexpected Termination of a Process assigned the requested Run Target happened.
    /// @error score::lcm::ExecErrc::kInvalidArguments if argument passed doesn't appear to be valid (e.g. after a software update, given Run Target doesn't exist anymore)
    /// @error score::lcm::ExecErrc::kCommunicationError if ControlClient can't communicate with Launch Manager (e.g. IPC link is down)
    /// @error score::lcm::ExecErrc::kAlreadyInState if the requested Run Target is already active
    /// @error score::lcm::ExecErrc::kInTransitionToSameState if there is already an ongoing request to activate requested Run Target
    /// @error score::lcm::ExecErrc::kInvalidTransition if activation of the requested Run Target is prohibited (e.g. Off Run Target)
    /// @error score::lcm::ExecErrc::kGeneralError if any other error occurs.
    ///
    /// @threadsafety{thread-safe}
    ///
    score::concurrency::InterruptibleFuture<void> ActivateRunTarget(std::string_view runTargetName) const noexcept;

   private:
    /// @brief Pointer to implementation (Pimpl), we use this pattern to provide ABI compatibility.
    std::unique_ptr<ControlClientImpl> control_client_impl_;
};

}  // namespace lcm

}  // namespace score

#endif  // CONTROL_CLIENT_H_
