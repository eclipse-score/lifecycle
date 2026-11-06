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

#include "score/mw/launch_manager/process_group_manager/details/component_factory.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/process_group_manager/details/component.hpp"
#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/process_force_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/empty_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/file_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/report_running_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/process_start_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/process_stop_action.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <chrono>
#include <variant>

namespace score::mw::lifecycle::internal
{

namespace
{

/// @brief A component which owns the actions it was created with.
/// @details Component only refers to its actions, so this keeps the actions alive for as long
///          as the component exists. Members are destroyed in reverse order, so the component is
///          destroyed before the actions it refers to.
class OwningComponent final : public IComponent
{
  public:
    OwningComponent(
        std::unique_ptr<const IStartAction> start_action,
        std::unique_ptr<const IStopAction> stop_action,
        std::unique_ptr<const IForceStopAction> force_stop_action,
        std::unique_ptr<const IReadyCondition> ready_condition,
        ISupervisionFactory& supervision_factory,
        configuration::ComponentConfig& config)
        : start_action_(std::move(start_action)),
          stop_action_(std::move(stop_action)),
          force_stop_action_(std::move(force_stop_action)),
          ready_condition_(std::move(ready_condition)),
          component_(*start_action_, *stop_action_, *force_stop_action_, *ready_condition_, supervision_factory, config)
    {
    }

    RequestResult activate(cpp::stop_token stop_token) override
    {
        return component_.activate(std::move(stop_token));
    }

    RequestResult deactivate(cpp::stop_token stop_token) override
    {
        return component_.deactivate(std::move(stop_token));
    }

    TerminationResult tryHandleTermination(int32_t status) override
    {
        return component_.tryHandleTermination(status);
    }

    [[nodiscard]] IdentifierHash getIdentifier() const override
    {
        return component_.getIdentifier();
    }

    [[nodiscard]] bool active() const override
    {
        return component_.active();
    }

  private:
    const std::unique_ptr<const IStartAction> start_action_;
    const std::unique_ptr<const IStopAction> stop_action_;
    const std::unique_ptr<const IForceStopAction> force_stop_action_;
    const std::unique_ptr<const IReadyCondition> ready_condition_;
    Component component_;
};

/// @brief Choose the ready condition for a component from its configuration.
/// @return The ready condition, or an error if the configured ready condition is not supported.
Result<std::unique_ptr<const IReadyCondition>> createReadyCondition(
    const configuration::ComponentConfig& config,
    osal::IProcess& launcher,
    const osal::IFileWaiter& file_waiter)
{
    const std::chrono::milliseconds ready_timeout{config.deployment_config.ready_timeout_ms};

    // A component can only have one ready condition. For a reporting process with a FileState
    // ready condition, only the file is waited for, and its kRunning report is ignored.
    if (const auto* const file_state =
            std::get_if<configuration::FileState>(&config.component_properties.ready_condition))
    {
        // req-id: comp_req__launch_man__rc_file_state
        return std::make_unique<const FileReadyCondition>(
            file_waiter, file_state->file_path, file_state->state, ready_timeout, file_state->polling_interval);
    }

    switch (std::get<configuration::ProcessState>(config.component_properties.ready_condition))
    {
        case configuration::ProcessState::Running:
            if (config.component_properties.application_profile.application_type ==
                configuration::ApplicationType::Native)
            {
                // A native process never reports kRunning, so it is ready as soon as it has started.
                return std::make_unique<const EmptyReadyCondition>();
            }
            return std::make_unique<const ReportRunningReadyCondition>(launcher, ready_timeout);

        case configuration::ProcessState::Terminated:
            // TODO: Waiting for termination is not available as an IReadyCondition yet.
            LM_LOG_ERROR() << config.name << ": ready condition 'Terminated' is not supported yet";
            return MakeUnexpected(ExecErrc::kNotImplemented);
    }

    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("All possible cases are covered");
}

}  // namespace

ComponentFactory::ComponentFactory(
    osal::IProcess& launcher,
    const osal::IFileWaiter& file_waiter,
    ISupervisionFactory& supervision_factory)
    : launcher_(launcher), file_waiter_(file_waiter), supervision_factory_(supervision_factory)
{
}

Result<std::unique_ptr<IComponent>> ComponentFactory::create(configuration::ComponentConfig& config) const
{
    Result<std::unique_ptr<const IReadyCondition>> ready_condition =
        createReadyCondition(config, launcher_, file_waiter_);
    if (!ready_condition.has_value())
    {
        return MakeUnexpected<std::unique_ptr<IComponent>>(ready_condition.error());
    }

    return std::make_unique<OwningComponent>(
        std::make_unique<const ProcessStartAction>(launcher_, config),
        std::make_unique<const ProcessStopAction>(launcher_),
        std::make_unique<const ProcessForceStopAction>(launcher_),
        std::move(ready_condition).value(),
        supervision_factory_,
        config);
}

}  // namespace score::mw::lifecycle::internal
