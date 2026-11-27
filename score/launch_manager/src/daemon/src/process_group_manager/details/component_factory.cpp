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
#include "score/mw/launch_manager/process_group_manager/details/process/process_force_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process/process_start_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process/process_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process/process_terminated_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process/report_running_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/empty_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/file_ready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/iready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/sequence_ready_condition.hpp"
#include <score/assert.hpp>
#include <type_traits>
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

/// @brief Create a ready condition which first waits for the kRunning report of the process, and only
///        then for @p condition. Both share the ready timeout.
std::unique_ptr<const IReadyCondition> afterReportedRunning(
    osal::IProcess& launcher,
    std::unique_ptr<const IReadyCondition> condition)
{
    std::vector<std::unique_ptr<const IReadyCondition>> stages;
    stages.push_back(std::make_unique<const ReportRunningReadyCondition>(launcher));
    stages.push_back(std::move(condition));
    return std::make_unique<const SequenceReadyCondition>(std::move(stages));
}

/// @brief Choose the ready condition for a component from its configuration.
/// @details A reporting process must report kRunning before any other ready condition is waited for.
///          A native process never reports kRunning.
/// @return The ready condition.
std::unique_ptr<const IReadyCondition> createReadyCondition(
    const configuration::ComponentConfig& config,
    osal::IProcess& launcher,
    const osal::IFileWaiter& file_waiter)
{
    const bool reporting = isReporting(config);
    const bool is_self_terminating = config.component_properties.application_profile.is_self_terminating;

    return std::visit(
        [reporting, is_self_terminating, &launcher, &file_waiter](
            const auto& ready_condition) -> std::unique_ptr<const IReadyCondition> {
            using T = std::decay_t<decltype(ready_condition)>;
            if constexpr (std::is_same_v<T, configuration::FileState>)
            {
                // req-id: comp_req__launch_man__rc_file_state
                auto file_ready_condition = std::make_unique<const FileReadyCondition>(
                    file_waiter,
                    ready_condition.file_path,
                    ready_condition.state,
                    ready_condition.polling_interval,
                    is_self_terminating);
                if (reporting)
                {
                    return afterReportedRunning(launcher, std::move(file_ready_condition));
                }
                return file_ready_condition;
            }
            else
            {
                static_assert(std::is_same_v<T, configuration::ProcessState>, "Unhandled ready condition type");
                switch (ready_condition)
                {
                    case configuration::ProcessState::Running:
                        if (reporting)
                        {
                            return std::make_unique<const ReportRunningReadyCondition>(launcher);
                        }
                        // A native process is ready as soon as it has started.
                        return std::make_unique<const EmptyReadyCondition>();

                    case configuration::ProcessState::Terminated:
                    {
                        auto terminated_ready_condition = std::make_unique<const ProcessTerminatedReadyCondition>();
                        if (reporting)
                        {
                            return afterReportedRunning(launcher, std::move(terminated_ready_condition));
                        }
                        return terminated_ready_condition;
                    }
                }

                SCORE_LANGUAGE_FUTURECPP_UNREACHABLE_MESSAGE("All possible cases are covered");
            }
        },
        config.component_properties.ready_condition);
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

}  // namespace score::mw::lifecycle::internal
