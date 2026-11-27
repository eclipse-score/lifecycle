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
#include "score/mw/launch_manager/process_group_manager/details/component.hpp"
#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include "score/result/result.h"
#include <memory>

namespace score::mw::lifecycle::internal
{

/// @brief Creates components, choosing the actions to use from each component's configuration.
/// @details Every component is started, stopped and force stopped as a POSIX process, using
///          ProcessStartAction, ProcessStopAction and ProcessForceStopAction. Started processes
///          are registered in the process map, so that their termination is reported to their
///          component.
///          The ready condition is chosen from the configured ready condition:
///          - \c FileState for a \c Native application: FileReadyCondition.
///          - \c FileState for any other application type: SequenceReadyCondition, which first waits
///            for kRunning (ReportRunningReadyCondition) and then for the file (FileReadyCondition).
///            Both share the ready timeout.
///          - \c Running for a \c Native application: EmptyReadyCondition.
///          - \c Running for any other application type: ReportRunningReadyCondition.
///          - \c Terminated: ProcessTerminatedReadyCondition, for every application type.
///
///          Each returned component owns its actions and shares ownership of its configuration,
///          so it stays valid without the factory. It still refers to the dependencies given to
///          the factory, so those must outlive the component.
class ComponentFactory final
{
  public:
    /// @brief Creates a new component factory.
    /// @param launcher Used by the process actions to start, stop and wait for POSIX processes.
    /// @param process_map Used by the process start action to report process terminations to
    ///                    their component.
    /// @param file_waiter Used by file-based ready conditions to wait for a file.
    /// @param supervision_factory Used to set up alive supervision for supervised components.
    ComponentFactory(
        osal::IProcess& launcher,
        SafeProcessMapInserter& process_map,
        const osal::IFileWaiter& file_waiter,
        ISupervisionFactory& supervision_factory);

    /// @brief Create a component from its configuration.
    /// @param config Configuration of the component; must not be null. The returned component
    ///               shares ownership of it. If the component is supervised, the alive supervision
    ///               connection is added to its environment variables.
    /// @return The created component instance.
    ///         The concrete type is returned, because the graph needs Component::forceStop().
    [[nodiscard]] Result<std::unique_ptr<Component>> create(
        std::shared_ptr<configuration::ComponentConfig> config) const;

  private:
    /// @brief Used by the process actions to start, stop and wait for POSIX processes.
    osal::IProcess& launcher_;

    /// @brief Used by the process start action to report process terminations to their component.
    SafeProcessMapInserter& process_map_;

    /// @brief Used by file-based ready conditions to wait for a file.
    const osal::IFileWaiter& file_waiter_;

    /// @brief Used to set up alive supervision for supervised components.
    ISupervisionFactory& supervision_factory_;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_COMPONENT_FACTORY_HPP_INCLUDED
