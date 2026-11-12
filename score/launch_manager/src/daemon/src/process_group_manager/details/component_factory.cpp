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
#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/empty_force_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/process_force_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/empty_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/file_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/report_running_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/empty_start_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/process_start_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/empty_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/process_stop_action.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include <chrono>

namespace score::mw::lifecycle::internal
{

namespace
{

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

Result<std::unique_ptr<IComponent>> ComponentFactory::create(
    std::shared_ptr<configuration::ComponentConfig> config) const
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(config != nullptr, "ComponentFactory requires a configuration");

    Result<std::unique_ptr<const IReadyCondition>> ready_condition =
        createReadyCondition(*config, launcher_, file_waiter_);
    if (!ready_condition.has_value())
    {
        return MakeUnexpected<std::unique_ptr<IComponent>>(ready_condition.error());
    }

    // The actions refer to the configuration, which is kept alive by the component they belong to.
    auto start_action = std::make_unique<const ProcessStartAction>(launcher_, *config);

    return std::make_unique<Component>(
        std::move(start_action),
        std::make_unique<const ProcessStopAction>(launcher_),
        std::make_unique<const ProcessForceStopAction>(launcher_),
        std::move(ready_condition).value(),
        supervision_factory_,
        std::move(config));
}

std::unique_ptr<IComponent> ComponentFactory::createRunTarget(const std::string& name) const
{
    // A Run Target has no configuration of its own. The component only needs a name; the
    // remaining defaults (native, not supervised, not self-terminating) are never used.
    auto config = std::make_shared<configuration::ComponentConfig>();
    config->name = name;

    return std::make_unique<Component>(
        std::make_unique<const EmptyStartAction>(),
        std::make_unique<const EmptyStopAction>(),
        std::make_unique<const EmptyForceStopAction>(),
        std::make_unique<const EmptyReadyCondition>(),
        supervision_factory_,
        std::move(config));
}

}  // namespace score::mw::lifecycle::internal
