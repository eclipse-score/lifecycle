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

#include "score/mw/launch_manager/control/control_provider.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"

namespace
{
using score::mw::com::InstanceSpecifier;
using score::mw::lifecycle::internal::LmControlSkeleton;

LmControlSkeleton create_skeleton()
{
    const auto instance_specifier_result =
        InstanceSpecifier::Create(std::string{"LaunchManager/StateManager/Instance"});
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        instance_specifier_result.has_value(), instance_specifier_result.error().Message().data());

    auto skeleton_result = LmControlSkeleton::Create(instance_specifier_result.value());
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(skeleton_result.has_value(), skeleton_result.error().Message().data());

    return std::move(skeleton_result).value();
}
}  // namespace

namespace score::mw::lifecycle::internal
{

ControlProvider::ControlProvider(IControllableGraph* graph) : skeleton_(create_skeleton()), graph_(graph)
{
    setup_activate_run_target();
    setup_get_active_run_target();
    setup_activation_result();
    offer_service();
}

void ControlProvider::setup_activate_run_target()
{
    const auto result = skeleton_.activate_run_target.RegisterHandler(
        [this](ActivateRunTargetResponse& response, const ActivateRunTargetRequest& request) {
            this->handle_activate_run_target(response, request);
        });
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(result.has_value(), result.error().Message().data());
}

void ControlProvider::handle_activate_run_target(
    ActivateRunTargetResponse& response,
    const ActivateRunTargetRequest& request)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
        request.mode == ActivationMode::kForced, "Only ActivationMode::kForced is implemented");

    const std::optional<IdentifierHash> new_state = IdentifierHash::if_exists(request.run_target_name.data());
    if (!new_state.has_value())
    {
        response = ActivateRunTargetResponse{
            status : RequestStatus::kRejected,
            rejection_reason : ExecErrc::kRunTargetDoesntExist
        };
        return;
    }

    const score::Result<void> result = graph_->set_requested_run_target(new_state.value());
    if (!result.has_value())
    {
        response = ActivateRunTargetResponse{
            status : RequestStatus::kRejected,
            rejection_reason : static_cast<ExecErrc>(*result.error())
        };
        return;
    }

    response = ActivateRunTargetResponse{status : RequestStatus::kAccepted};
}

void ControlProvider::setup_get_active_run_target()
{
    const auto result = skeleton_.get_active_run_target.RegisterHandler([this](GetActiveRunTargetResponse& response) {
        this->handle_get_active_run_target(response);
    });
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(result.has_value(), result.error().Message().data());
}

void ControlProvider::handle_get_active_run_target(GetActiveRunTargetResponse& response)
{
    const score::Result<IdentifierHash> result = graph_->get_active_run_target();
    if (!result.has_value())
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
            static_cast<ExecErrc>(*result.error()) == ExecErrc::kActivationInProgress,
            "Impossible to communicate errors other than ExecErrc::kActivationInProgress to the client");

        response = GetActiveRunTargetResponse{status : QueryStatus::kNotAvailable, run_target : RunTargetName("")};
        return;
    }

    const std::lock_guard<std::mutex> lock(IdentifierHash::get_registry_mutex());
    const std::string& name = IdentifierHash::get_registry()[result.value().data()];

    response = GetActiveRunTargetResponse{status : QueryStatus::kAvailable, run_target : RunTargetName(name)};
}

void ControlProvider::setup_activation_result()
{
    graph_->watch_active_run_target([this](IdentifierHash state, RunTargetActivationSource source) {
        this->handle_activation_result(state, source);
    });
}

void ControlProvider::handle_activation_result(IdentifierHash state, RunTargetActivationSource source)
{
    auto allocate_result = skeleton_.activation_result.Allocate();
    if (!allocate_result.has_value())
    {
        LM_LOG_ERROR() << "Failed to allocate space to send the activation result to the state manager:"
                          "check that the mw::com configuration is correct";
        return;
    }

    ActivationResult* event = allocate_result.value().Get();
    {
        const std::lock_guard<std::mutex> lock(IdentifierHash::get_registry_mutex());
        const auto& registry = IdentifierHash::get_registry();
        const auto it = registry.find(state.data());
        SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
            it != registry.end(), "IdentifierHash does not correspond to an existing name");
        event->activated_run_target = RunTargetName(it->second);
    }
    event->activation_source = source;

    const auto send_result = skeleton_.activation_result.Send(std::move(allocate_result.value()));
    if (!send_result.has_value())
    {
        LM_LOG_ERROR() << "Failed to send the activation result to the state manager";
        return;
    }

    LM_LOG_DEBUG() << "Sent the activation result to the state manager";
}

void ControlProvider::offer_service()
{
    const auto result = skeleton_.OfferService();
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(result.has_value(), result.error().Message().data());

    // Workaround for https://github.com/eclipse-score/communication/issues/1064.
    // This should be removed once the above issue is solved.
    for (int fd = 0; fd < 16; fd++)
    {
        switch (fd)
        {
            case STDIN_FILENO:
            case STDOUT_FILENO:
            case STDERR_FILENO:
            case osal::IpcCommsSync::sync_fd:
                break;
            default:
                int flags = fcntl(fd, F_GETFD);
                if (flags == -1)
                {
                    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
                        errno == EBADF, "fcntl F_GETFD failed with unexpected error");
                }
                else
                {
                    flags |= FD_CLOEXEC;
                    const auto result = fcntl(fd, F_SETFD, flags);
                    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(result != -1, "fcntl F_SETFD failed");
                }
                break;
        }
    }
}

}  // namespace score::mw::lifecycle::internal
