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

// Holds everything that self-references via a captured `this`: the three callbacks registered
// below all capture `Impl*`, and that address has to stay fixed for as long as they are
// registered (there is no unregister path). `ControlProvider` itself only owns a
// `unique_ptr<Impl>`, so it stays freely movable while `Impl`'s address never moves.
class ControlProvider::Impl
{
  public:
    Impl(LmControlSkeleton skeleton, IRunTargetControl* graph) noexcept : skeleton_(std::move(skeleton)), graph_(graph)
    {
    }

    /// @brief Register the handler for activate_run_target.
    Result<void> setupActivateRunTarget() noexcept;

    /// @brief Handle an activate_run_target request.
    void handleActivateRunTarget(ActivateRunTargetResponse& response, const ActivateRunTargetRequest& request) noexcept;

    /// @brief Register the handler for get_active_run_target.
    Result<void> setupGetActiveRunTarget() noexcept;

    /// @brief Handle a get_active_run_target request.
    void handleGetActiveRunTarget(GetActiveRunTargetResponse& response) noexcept;

    /// @brief Register the handler for activation_result.
    Result<void> setupActivationResult() noexcept;

    /// @brief Handle an activation_result event.
    void handleActivationResult(IdentifierHash state, RunTargetActivationSource source) noexcept;

    /// @brief Make the service available to clients.
    Result<void> offerService() noexcept;

  private:
    /// @brief The external `mw::com` interface.
    LmControlSkeleton skeleton_;

    /// @brief The underlying graph implementation.
    IRunTargetControl* graph_;
};

Result<ControlProvider> ControlProvider::Create(IRunTargetControl* graph) noexcept
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

    // `unique_ptr` rather than the previous raw `new`: on any of the failure paths below, this
    // is freed automatically instead of leaking (the previous version returned
    // `MakeUnexpected(...)` without ever deleting the raw pointer it had just allocated).
    auto impl = std::make_unique<ControlProvider::Impl>(std::move(skeleton), graph);

    const Result<void> setup_activate_run_target_result = impl->setupActivateRunTarget();
    if (!setup_activate_run_target_result.has_value())
    {
        return MakeUnexpected(static_cast<ExecErrc>(*setup_activate_run_target_result.error()));
    }

    const Result<void> setup_get_active_run_target_result = impl->setupGetActiveRunTarget();
    if (!setup_get_active_run_target_result.has_value())
    {
        return MakeUnexpected(static_cast<ExecErrc>(*setup_get_active_run_target_result.error()));
    }

    const Result<void> setup_activation_result_result = impl->setupActivationResult();
    if (!setup_activation_result_result.has_value())
    {
        return MakeUnexpected(static_cast<ExecErrc>(*setup_activation_result_result.error()));
    }

    const Result<void> offer_service_result = impl->offerService();
    if (!offer_service_result.has_value())
    {
        return MakeUnexpected(static_cast<ExecErrc>(*offer_service_result.error()));
    }

    return ControlProvider{std::move(impl)};
}

ControlProvider::ControlProvider(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl))
{
}

// Defined here rather than defaulted in the header: `Impl` is only a complete type from this
// point in the file onward, and a deleter/mover for `unique_ptr<Impl>` needs that completeness.
ControlProvider::ControlProvider(ControlProvider&&) noexcept = default;
ControlProvider& ControlProvider::operator=(ControlProvider&&) noexcept = default;
ControlProvider::~ControlProvider() = default;

Result<void> ControlProvider::Impl::setupActivateRunTarget() noexcept
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

void ControlProvider::Impl::handleActivateRunTarget(
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

Result<void> ControlProvider::Impl::setupGetActiveRunTarget() noexcept
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

void ControlProvider::Impl::handleGetActiveRunTarget(GetActiveRunTargetResponse& response) noexcept
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

    const std::lock_guard<std::mutex> lock(IdentifierHash::get_registry_mutex());
    const std::string& name = IdentifierHash::get_registry()[result.value().data()];

    response = GetActiveRunTargetResponse{status : QueryStatus::kAvailable, run_target : RunTargetName(name)};
}

Result<void> ControlProvider::Impl::setupActivationResult() noexcept
{
    graph_->registerActiveRunTargetCallback([this](IdentifierHash state, RunTargetActivationSource source) {
        this->handleActivationResult(state, source);
    });

    return {};
}

void ControlProvider::Impl::handleActivationResult(IdentifierHash state, RunTargetActivationSource source) noexcept
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

Result<void> ControlProvider::Impl::offerService() noexcept
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
