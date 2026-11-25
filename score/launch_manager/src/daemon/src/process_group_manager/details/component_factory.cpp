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
#include "score/mw/launch_manager/process_group_manager/details/component.hpp"
#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/empty_force_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/process_force_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/empty_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/file_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/iready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/process_terminated_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/report_running_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/sequence_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/empty_start_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/process_start_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/empty_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/process_stop_action.hpp"
#include "score/mw/lifecycle/execution_error.h"
#include "score/result/error.h"
#include <score/assert.hpp>
#include <chrono>
#include <utility>
#include <variant>
#include <vector>

namespace score::mw::lifecycle::internal
{

namespace
{

/// @brief Whether the process of a component reports kRunning through the lifecycle API.
bool isReporting(const configuration::ComponentConfig& config) noexcept
{
    switch (config.component_properties.application_profile.application_type)
    {
        case configuration::ApplicationType::Reporting:
        case configuration::ApplicationType::ReportingAndSupervised:
        case configuration::ApplicationType::StateManager:
            return true;
        case configuration::ApplicationType::Native:
            return false;
    }

    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("All possible cases are covered");
}

/// @brief Choose the ready condition for a component from its configuration.
/// @return The ready condition.
std::unique_ptr<const IReadyCondition> createReadyCondition(
    const configuration::ComponentConfig& config,
    osal::IProcess& launcher,
    const osal::IFileWaiter& file_waiter)
{
    if (const auto* const file_state =
            std::get_if<configuration::FileState>(&config.component_properties.ready_condition))
    {
        // req-id: comp_req__launch_man__rc_file_state
        auto file_ready_condition = std::make_unique<const FileReadyCondition>(
            file_waiter, file_state->file_path, file_state->state, file_state->polling_interval);

        if (!isReporting(config))
        {
            return file_ready_condition;
        }

        // A reporting process first reports kRunning, and only then the file is waited for. Both
        // share the ready timeout.
        std::vector<std::unique_ptr<const IReadyCondition>> stages;
        stages.push_back(std::make_unique<const ReportRunningReadyCondition>(launcher));
        stages.push_back(std::move(file_ready_condition));
        return std::make_unique<const SequenceReadyCondition>(std::move(stages));
    }

    switch (std::get<configuration::ProcessState>(config.component_properties.ready_condition))
    {
        case configuration::ProcessState::Running:
            if (!isReporting(config))
            {
                // A native process never reports kRunning, so it is ready as soon as it has started.
                return std::make_unique<const EmptyReadyCondition>();
            }
            return std::make_unique<const ReportRunningReadyCondition>(launcher);

        case configuration::ProcessState::Terminated:
            // Only the termination is waited for, for every application type. A reporting process
            // is not waited for to report kRunning.
            return std::make_unique<const ProcessTerminatedReadyCondition>();
    }

    SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("All possible cases are covered");
}

}  // namespace

ComponentFactory::ComponentFactory(
    osal::IProcess& launcher,
    SafeProcessMapInserter& process_map,
    const osal::IFileWaiter& file_waiter,
    ISupervisionFactory& supervision_factory)
    : launcher_(launcher),
      process_map_(process_map),
      file_waiter_(file_waiter),
      supervision_factory_(supervision_factory)
{
}

Result<std::unique_ptr<Component>> ComponentFactory::create(
    std::shared_ptr<configuration::ComponentConfig> config) const
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(config != nullptr, "ComponentFactory requires a configuration");

    std::unique_ptr<const IReadyCondition> ready_condition = createReadyCondition(*config, launcher_, file_waiter_);

    // The actions refer to the configuration, which is kept alive by the component they belong to.
    auto start_action = std::make_unique<const ProcessStartAction>(launcher_, process_map_, *config);

    return std::make_unique<Component>(
        std::move(start_action),
        std::make_unique<const ProcessStopAction>(launcher_),
        std::make_unique<const ProcessForceStopAction>(launcher_),
        std::move(ready_condition),
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
