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

#ifndef SCORE_LCM_CONTROL_PROVIDER
#define SCORE_LCM_CONTROL_PROVIDER

#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/launch_manager/process_group_manager/irun_target_control.hpp"
#include "score/mw/lifecycle/details/lm_control_service.h"

#include <memory>
#include <mutex>
#include <optional>

namespace score::mw::lifecycle::internal
{

/// @brief Default specifier factory that creates the production mw::com instance specifier.
struct LmControlSpecifierFactory
{
    static Result<com::InstanceSpecifier> Create()
    {
        return com::InstanceSpecifier::Create(std::string{"LaunchManager/StateManager/Instance"});
    }
};

/// @brief Provides the mw::com service for state managers to connect to.
/// @details This cannot be moved, because the mw::com callbacks reference
//           the ControlProvider at its original location. It is therefore
//           handed out as a `std::unique_ptr`, which keeps that address fixed.
///
/// @tparam SkeletonT        The mw::com skeleton type. Defaults to LmControlSkeleton (production).
///                          Tests inject a fake skeleton to exercise handler logic without a
///                          real mw::com runtime.
/// @tparam SpecifierFactory A type with a static Create() that returns the mw::com instance
///                          specifier. Defaults to LmControlSpecifierFactory (production).
template <typename SkeletonT, typename SpecifierFactory>
class BasicControlProvider
{
  public:
    /// @brief Named constructor for BasicControlProvider.
    static Result<std::unique_ptr<BasicControlProvider>> Create(IRunTargetControl* graph) noexcept
    {
        const Result<com::InstanceSpecifier> instance_specifier_result = SpecifierFactory::Create();
        if (!instance_specifier_result.has_value())
        {
            LM_LOG_ERROR() << "Failed to create mw::com instance specifier:"
                           << instance_specifier_result.error().Message();
            return MakeUnexpected(ExecErrc::kCommunicationError);
        }

        Result<SkeletonT> skeleton_result = SkeletonT::Create(instance_specifier_result.value());
        if (!skeleton_result.has_value())
        {
            LM_LOG_ERROR() << "Failed to create LmControlSkeleton:" << skeleton_result.error().Message();
            return MakeUnexpected(ExecErrc::kCommunicationError);
        }
        SkeletonT skeleton = std::move(skeleton_result).value();

        // Owned by a `unique_ptr` so that it is freed on any of the failure paths below. Not
        // `std::make_unique`, because the constructor is private.
        std::unique_ptr<BasicControlProvider> control_provider{new BasicControlProvider{std::move(skeleton), graph}};

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

        control_provider->setupActivationResult();

        const Result<void> offer_service_result = control_provider->offerService();
        if (!offer_service_result.has_value())
        {
            return MakeUnexpected(static_cast<ExecErrc>(*offer_service_result.error()));
        }

        return control_provider;
    }

    ~BasicControlProvider() = default;

    // Cannot be moved because callbacks capture the BasicControlProvider by reference.
    BasicControlProvider(const BasicControlProvider&) = delete;
    BasicControlProvider(BasicControlProvider&&) = delete;
    BasicControlProvider& operator=(const BasicControlProvider&) = delete;
    BasicControlProvider operator=(BasicControlProvider&&) = delete;

  private:
    explicit BasicControlProvider(SkeletonT skeleton, IRunTargetControl* graph) noexcept
        : skeleton_(std::move(skeleton)), graph_(graph)
    {
    }

    /// @brief Register the handler for activate_run_target.
    Result<void> setupActivateRunTarget() noexcept
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

    /// @brief Handle an activate_run_target request.
    void handleActivateRunTarget(ActivateRunTargetResponse& response, const ActivateRunTargetRequest& request) noexcept
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
            LM_LOG_ERROR() << "Activation request failed: Run Target" << request.run_target_name << "does not exist";

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

    /// @brief Register the handler for get_active_run_target.
    Result<void> setupGetActiveRunTarget() noexcept
    {
        const auto result =
            skeleton_.get_active_run_target.RegisterHandler([this](GetActiveRunTargetResponse& response) {
                this->handleGetActiveRunTarget(response);
            });

        if (!result.has_value())
        {
            LM_LOG_ERROR() << "Failed to register handler for get_active_run_target:" << result.error().Message();
            return MakeUnexpected(ExecErrc::kCommunicationError);
        }

        return {};
    }

    /// @brief Handle a get_active_run_target request.
    void handleGetActiveRunTarget(GetActiveRunTargetResponse& response) noexcept
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

    /// @brief Register the handler for activation_result.
    void setupActivationResult() noexcept
    {
        graph_->registerActiveRunTargetCallback([this](IdentifierHash state, RunTargetActivationSource source) {
            this->handleActivationResult(state, source);
        });
    }

    /// @brief Handle an activation_result event.
    void handleActivationResult(IdentifierHash state, RunTargetActivationSource source) noexcept
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

    /// @brief Make the service available to clients.
    Result<void> offerService() noexcept
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

    /// @brief The external `mw::com` interface.
    SkeletonT skeleton_;

    /// @brief The underlying graph implementation.
    IRunTargetControl* graph_;
};

/// @brief Production alias: BasicControlProvider wired to the real mw::com skeleton.
using ControlProvider = BasicControlProvider<LmControlSkeleton, LmControlSpecifierFactory>;

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_CONTROL_PROVIDER
