/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#ifndef ALIVEWORKERFACTORY_HPP_INCLUDED
#define ALIVEWORKERFACTORY_HPP_INCLUDED

#include <memory>

#include "score/mw/launch_manager/alive_monitor/details/factory/IAliveWorkerFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEventReader.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include <string>
#include <vector>

namespace score::mw::lifecycle
{
class ControlClient;
}  // namespace score::mw::lifecycle

namespace score::mw::lifecycle::internal::saf::factory
{

/// @brief Alive worker factory
/// @details Provides methods to create worker objects
///          and establishes required links between the worker objects automatically.
class AliveWorkerFactory : public IAliveWorkerFactory
{
  public:
    /// @brief Constructor
    explicit AliveWorkerFactory();

    /// @brief Destructor
    /* RULECHECKER_comment(0, 5, check_min_instructions, "Default destructor is not provided\
       a function body", true_no_defect) */
    ~AliveWorkerFactory() override = default;

    /// @brief No Copy Constructor
    AliveWorkerFactory(const AliveWorkerFactory&) = delete;
    /// @brief No Copy Assignment
    AliveWorkerFactory& operator=(const AliveWorkerFactory&) = delete;
    /// @brief No Move Constructor
    AliveWorkerFactory(AliveWorkerFactory&&) = delete;
    /// @brief No Move Assignment
    AliveWorkerFactory& operator=(AliveWorkerFactory&&) = delete;

    /// @brief Refer to the description of the base class (IAliveWorkerFactory)
    bool createObservableEvent(
        std::vector<ifexm::ObservableEvent>& events,
        const IdentifierHash component_id,
        ifexm::ObservableEventReader& event_reader_) override;

    /// Refer to the description of the base class (IAliveWorkerFactory)
    bool createAliveIfIpc(
        std::vector<ifappl::CheckpointIpcServer>& servers,
        const IdentifierHash component_id,
        const uid_t uid) override;

    /// Refer to the description of the base class (IAliveWorkerFactory)
    bool createAliveIf(
        std::vector<ifappl::MonitorIfDaemon>& interfaces,
        ifappl::CheckpointIpcServer& ipc_server,
        ifexm::ObservableEvent& event) override;

    /// Refer to the description of the base class (IAliveWorkerFactory)
    bool createSupervisionCheckpoint(
        std::vector<ifappl::Checkpoint>& checkpoints,
        ifappl::MonitorIfDaemon& interface,
        const ifexm::ObservableEvent& event,
        const IdentifierHash component_id) override;

    /// Refer to the description of the base class (IAliveWorkerFactory)
    bool createAliveSupervision(
        std::vector<supervision::Alive>& supervisions,
        ifappl::Checkpoint& checkpoint,
        ifexm::ObservableEvent& event,
        const std::shared_ptr<IRecoveryClient> recovery_client,
        const IdentifierHash component_id,
        const ComponentAliveSupervision component_config) override;

  private:
    /// @brief Create IPC Channel with uid-based access permission
    /// @details Only the given uid will ge granted r/w access, no group will be granted access
    /// @param[in,out] f_ipcServer_r The IPC server object
    /// @param[in] f_ipcPath_r The name of the IPC channel
    /// @param[in] f_uid The uid that will be assigned r/w permissions for ipc communication
    /// @return True if creation was successful, else false
    bool initIpcServerWithUidBasedAccess(
        ifappl::CheckpointIpcServer& f_ipcServer_r,
        const std::string& f_ipcPath_r,
        const std::int32_t f_uid) noexcept(false);
};

}  // namespace score::mw::lifecycle::internal::saf::factory

#endif
