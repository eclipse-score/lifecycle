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

#ifndef SCORE_LCM_COMPONENT_HPP_INCLUDED
#define SCORE_LCM_COMPONENT_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/ialive_supervision_handle.hpp"
#include "score/mw/launch_manager/alive_monitor/isupervision_factory.hpp"
#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/iforce_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/iready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/istart_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/istop_action.hpp"
#include <memory>
#include <mutex>
#include <variant>

namespace score::mw::lifecycle::internal
{

/// @brief Manages and tracks the state of a resource.
class Component final : public IComponent
{
  public:
    /// @brief Creates a new component from its configuration and the given actions.
    /// @param start_action How to start the resource.
    /// @param stop_action How to gracefully stop the resource.
    /// @param force_stop_action How to forcefully stop the resource.
    /// @param ready_condition How to decide when the resource has finished its startup.
    /// @param supervision_factory Factory used to construct alive supervision.
    /// @param config Configuration of the component.
    Component(
        std::unique_ptr<IStartAction>&& start_action,
        std::unique_ptr<IStopAction>&& stop_action,
        std::unique_ptr<IForceStopAction>&& force_stop_action,
        std::unique_ptr<IReadyCondition>&& ready_condition,
        ISupervisionFactory& supervision_factory,
        configuration::ComponentConfig&& config);

    /// @brief Start the resource and set the component to active.
    /// @param stop_token Token which can be used to interrupt the activation.
    /// @return Whether the component activation succeeded or failed.
    RequestResult activate(cpp::stop_token stop_token) override;

    /// @brief Stop the resource and set the component to inactive.
    /// @param stop_token Token which can be used to interrupt the deactivation.
    /// @return Whether the component deactivation succeeded or failed.
    RequestResult deactivate(cpp::stop_token stop_token) override;

    /// @brief Notify the component that a POSIX process has terminated.
    /// @param status Exit code of the process.
    /// @return Whether the termination is ignored or means the component failed.
    TerminationResult tryHandleTermination(int32_t status) override;

    /// @brief Return the name of the component.
    /// @return The name of the component.
    [[nodiscard]] IdentifierHash getIdentifier() const override;

    /// @brief Return whether the component is active.
    /// @return Whether the component is active.
    [[nodiscard]] bool active() const override;

    /// @brief Return the component's start action.
    [[nodiscard]] const IStartAction& startAction() const;

    /// @brief Return the component's stop action.
    [[nodiscard]] const IStopAction& stopAction() const;

    /// @brief Return the component's force-stop action.
    [[nodiscard]] const IForceStopAction& forceStopAction() const;

    /// @brief Return the component's ready condition.
    [[nodiscard]] const IReadyCondition& readyCondition() const;

    /// @brief Return the component's configuration.
    [[nodiscard]] const configuration::ComponentConfig& config() const;

  private:
    /// @brief How to start the resource.
    const std::unique_ptr<IStartAction> start_action_;

    /// @brief How to gracefully stop the resource.
    const std::unique_ptr<IStopAction> stop_action_;

    /// @brief How to forcefully stop the resource.
    const std::unique_ptr<IForceStopAction> force_stop_action_;

    /// @brief How to decide when the resource has finished its startup.
    const std::unique_ptr<IReadyCondition> ready_condition_;

    /// @brief Configuration of the component.
    configuration::ComponentConfig config_;

    /// @brief Alive supervision of the resource, or nullptr if it is not supervised.
    const std::unique_ptr<IAliveSupervisionHandle> supervision_handle_;

    struct TerminatedState final
    {
    };

    struct StartingState final
    {
        Handle handle_;
        cpp::stop_source stop_activation_;
    };

    struct ReadyState final
    {
        Handle handle_;
    };

    struct TerminatingState final
    {
        Handle handle_;
    };

    struct FaultState final
    {
        Handle handle_;
    };

    using State = std::variant<TerminatedState, StartingState, ReadyState, TerminatingState, FaultState>;

    /// @brief Protects the state from concurrent modifications.
    mutable std::mutex state_mutex_;

    /// @brief The current state of the component.
    State state_;

    /// @brief Start alive supervision now, if the component is supervised.
    [[nodiscard]] bool activateSupervision() const;

    /// @brief Stop alive supervision now, if the component is supervised.
    [[nodiscard]] bool deactivateSupervision() const;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_COMPONENT_HPP_INCLUDED
