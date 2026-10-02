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

#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/iforce_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/iready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/istart_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/istop_action.hpp"
#include <score/span.hpp>
#include <functional>
#include <vector>

namespace score::mw::lifecycle::internal
{

/// @brief Manages and tracks the state of a resource.
class Component final : public IComponent
{
  public:
    /// @brief Creates a new component from the given actions.
    /// @param start_action How to start the resource.
    /// @param stop_action How to gracefully stop the resource.
    /// @param force_stop_action How to forcefully stop the resource.
    /// @param ready_conditions How to decide when the resource has finished its startup.
    /// @param identifier Name of the component.
    Component(
        const IStartAction& start_action,
        const IStopAction& stop_action,
        const IForceStopAction& force_stop_action,
        cpp::span<std::reference_wrapper<const IReadyCondition>> ready_conditions =
            cpp::span<std::reference_wrapper<const IReadyCondition>>{},
        IdentifierHash identifier = IdentifierHash{});

    /// @brief Start the resource and set the component to active.
    /// @param stop_token Token which can be used to interrupt the activation.
    /// @return Whether the component activation was successful, is waiting for another
    ///         thread, or an error was encountered.
    RequestResult activate(cpp::stop_token stop_token) override;

    /// @brief Stop the resource and set the component to inactive.
    /// @param stop_token Token which can be used to interrupt the deactivation.
    /// @return Whether the component deactivation was successful, is waiting for another
    ///         thread, or an error was encountered.
    RequestResult deactivate(cpp::stop_token stop_token) override;

    /// @brief Notify the component that a POSIX process has terminated.
    /// @param status Exit code of the process.
    /// @return Whether the component activation was successful, is waiting for another
    ///         thread, or an error was encountered.
    RequestResult tryHandleTermination(int32_t status) override;

    /// @brief Return the name of the component.
    /// @return The name of the component.
    [[nodiscard]] IdentifierHash getIdentifier() const override;

    /// @brief Return whether the component is active.
    /// @return Whether the component is active.
    [[nodiscard]] bool active() const override;

  private:
    /// @brief The resource being managed.
    std::optional<Handle> handle_;

    /// @brief How to start the resource.
    const IStartAction& start_action_;

    /// @brief How to gracefully stop the resource.
    const IStopAction& stop_action_;

    /// @brief How to forcefully stop the resource.
    const IForceStopAction& force_stop_action_;

    /// @brief How to decide when the resource has finished its startup.
    const cpp::span<std::reference_wrapper<const IReadyCondition>> ready_conditions_;

    /// @brief Name of the component.
    IdentifierHash identifier_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_COMPONENT_HPP_INCLUDED
