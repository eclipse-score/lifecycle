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

#ifndef SCORE_LCM_COMPONENT_FACTORY_HPP_INCLUDED
#define SCORE_LCM_COMPONENT_FACTORY_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/isupervision_factory.hpp"
#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/mw/launch_manager/osal/ifile_waiter.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include "score/result/result.h"
#include <memory>
#include <string>

namespace score::mw::lifecycle::internal
{

/// @brief Creates components, choosing the actions to use from each component's configuration.
/// @details Every component is started, stopped and force stopped as a POSIX process, using
///          ProcessStartAction, ProcessStopAction and ProcessForceStopAction.
///          The ready condition is chosen from the configured ready condition:
///          - \c FileState: FileReadyCondition.
///          - \c Running for a \c Native application: EmptyReadyCondition.
///          - \c Running for any other application type: ReportRunningReadyCondition.
///          - \c Terminated: not supported yet, an error is returned.
///
///          Run Targets are created with createRunTarget(), using only empty actions.
///
///          Each returned component owns its actions and shares ownership of its configuration,
///          so it stays valid without the factory. It still refers to the dependencies given to
///          the factory, so those must outlive the component.
class ComponentFactory final
{
  public:
    /// @brief Creates a new component factory.
    /// @param launcher Used by the process actions to start, stop and wait for POSIX processes.
    /// @param file_waiter Used by file-based ready conditions to wait for a file.
    /// @param supervision_factory Used to set up alive supervision for supervised components.
    ComponentFactory(
        osal::IProcess& launcher,
        const osal::IFileWaiter& file_waiter,
        ISupervisionFactory& supervision_factory);

    /// @brief Create a component from its configuration.
    /// @param config Configuration of the component; must not be null. The returned component
    ///               shares ownership of it. If the component is supervised, the alive supervision
    ///               connection is added to its environment variables.
    /// @return The new, inactive component, or an error if its configuration is not supported:
    ///         - \c ExecErrc::kNotImplemented if the ready condition is \c ProcessState::Terminated.
    [[nodiscard]] Result<std::unique_ptr<IComponent>> create(
        std::shared_ptr<configuration::ComponentConfig> config) const;

    /// @brief Create a component for a Run Target.
    /// @details A Run Target does not manage any resource. It only exists to depend on other
    ///          components, so all of its actions and its ready condition are empty. It is active
    ///          as soon as it is activated, and inactive as soon as it is deactivated.
    /// @param name Name of the Run Target, used as the component's identifier.
    /// @return The new, inactive component.
    [[nodiscard]] std::unique_ptr<IComponent> createRunTarget(const std::string& name) const;

  private:
    /// @brief Used by the process actions to start, stop and wait for POSIX processes.
    osal::IProcess& launcher_;

    /// @brief Used by file-based ready conditions to wait for a file.
    const osal::IFileWaiter& file_waiter_;

    /// @brief Used to set up alive supervision for supervised components.
    ISupervisionFactory& supervision_factory_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_COMPONENT_FACTORY_HPP_INCLUDED
