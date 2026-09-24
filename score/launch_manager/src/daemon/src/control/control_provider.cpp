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

namespace score::mw::lifecycle::internal
{

Result<ControlProvider*> ControlProvider::Create(IRunTargetControl* graph) noexcept
{
    const Result<com::InstanceSpecifier> instance_specifier_result =
        com::InstanceSpecifier::Create(std::string{"LaunchManager/StateManager/Instance"});
    if (!instance_specifier_result.has_value())
    {
        LM_LOG_ERROR() << "Failed to create mw::com instance specifier:" << instance_specifier_result.error().Message();
        return MakeUnexpected(ExecErrc::kCommunicationError);
    }

    Result<LmControlSkeleton> skeleton_result = LmControlSkeleton::Create(instance_specifier_result.value());
    if (!skeleton_result.has_value())
    {
        LM_LOG_ERROR() << "Failed to create LmControlSkeleton:" << skeleton_result.error().Message();
        return MakeUnexpected(ExecErrc::kCommunicationError);
    }
    LmControlSkeleton skeleton = std::move(skeleton_result).value();

    auto* control_provider = new ControlProvider{std::move(skeleton), graph};

    const Result<void> setup_activate_run_target_result = control_provider->setupActivateRunTarget();
    if (!setup_activate_run_target_result.has_value())
    {
        return MakeUnexpected(static_cast<ExecErrc>(*setup_activate_run_target_result.error()));
    }

    const Result<void> setup_get_active_run_target_result = control_provider->setupGetActiveRunTarget();
    if (!setup_get_active_run_target_result.has_value())
    {
        return MakeUnexpected(static_cast<ExecErrc>(*setup_get_active_run_target_result.error()));
    }

    const Result<void> setup_activation_result_result = control_provider->setupActivationResult();
    if (!setup_activation_result_result.has_value())
    {
        return MakeUnexpected(static_cast<ExecErrc>(*setup_activation_result_result.error()));
    }

    const Result<void> offer_service_result = control_provider->offerService();
    if (!offer_service_result.has_value())
    {
        return MakeUnexpected(static_cast<ExecErrc>(*offer_service_result.error()));
    }

    return control_provider;
}

ControlProvider::ControlProvider(LmControlSkeleton skeleton, IRunTargetControl* graph) noexcept
    : skeleton_(std::move(skeleton)), graph_(graph)
{
}

Result<void> ControlProvider::setupActivateRunTarget() noexcept
{
    const auto result = skeleton_.activate_run_target.RegisterHandler(
        [this](ActivateRunTargetResponse& response, const ActivateRunTargetRequest& request) {
            this->handleActivateRunTarget(response, request);
        });

    if (!result.has_value())
    {
        LM_LOG_ERROR() << "Failed to register handler for activate_run_target:" << result.error().Message();
        return MakeUnexpected(ExecErrc::kCommunicationError);
    }

    return {};
}

void ControlProvider::handleActivateRunTarget(
    ActivateRunTargetResponse& response,
    const ActivateRunTargetRequest& request) noexcept
{
    // See https://github.com/eclipse-score/lifecycle/issues/643.
    if (request.mode != ActivationMode::kForced)
    {
        LM_LOG_ERROR() << "Activation request failed: queued activation is not yet implemented";

        response =
        ActivateRunTargetResponse{status : RequestStatus::kRejected, rejection_reason : ExecErrc::kNotImplemented};
        return;
    }

    const std::optional<IdentifierHash> new_state = IdentifierHash::if_exists(request.run_target_name.data());
    if (!new_state.has_value())
    {
        LM_LOG_ERROR() << "Activation request failed: run target" << request.run_target_name << "does not exist";

        response = ActivateRunTargetResponse{
            status : RequestStatus::kRejected,
            rejection_reason : ExecErrc::kRunTargetDoesntExist
        };
        return;
    }

    const score::Result<void> result = graph_->setRequestedRunTarget(new_state.value());
    if (!result.has_value())
    {
        LM_LOG_ERROR() << "Activation request failed:" << result.error().Message();

        response = ActivateRunTargetResponse{
            status : RequestStatus::kRejected,
            rejection_reason : static_cast<ExecErrc>(*result.error())
        };
        return;
    }

    response = ActivateRunTargetResponse{status : RequestStatus::kAccepted};
}

Result<void> ControlProvider::setupGetActiveRunTarget() noexcept
{
    const auto result = skeleton_.get_active_run_target.RegisterHandler([this](GetActiveRunTargetResponse& response) {
        this->handleGetActiveRunTarget(response);
    });

    if (!result.has_value())
    {
        LM_LOG_ERROR() << "Failed to register handler for get_active_run_target:" << result.error().Message();
        return MakeUnexpected(ExecErrc::kCommunicationError);
    }

    return {};
}

void ControlProvider::handleGetActiveRunTarget(GetActiveRunTargetResponse& response) noexcept
{
    const score::Result<IdentifierHash> result = graph_->getActiveRunTarget();
    if (!result.has_value())
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
            static_cast<ExecErrc>(*result.error()) == ExecErrc::kActivationInProgress,
            "Impossible to communicate errors other than ExecErrc::kActivationInProgress to the client");

        response = GetActiveRunTargetResponse{status : QueryStatus::kNotAvailable, run_target : RunTargetName("")};
        return;
    }

    const RunTargetName run_target{result.value().get_name()};

    response = GetActiveRunTargetResponse{status : QueryStatus::kAvailable, run_target};
}

Result<void> ControlProvider::setupActivationResult() noexcept
{
    graph_->registerActiveRunTargetCallback([this](IdentifierHash state, RunTargetActivationSource source) {
        this->handleActivationResult(state, source);
    });

    return {};
}

void ControlProvider::handleActivationResult(IdentifierHash state, RunTargetActivationSource source) noexcept
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

Result<void> ControlProvider::offerService() noexcept
{
    const auto result = skeleton_.OfferService();
    if (!result.has_value())
    {
        LM_LOG_ERROR() << "Failed to offer mw::com service:" << result.error().Message();
        return MakeUnexpected(ExecErrc::kCommunicationError);
    }

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

    return {};
}

}  // namespace score::mw::lifecycle::internal
