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


#ifndef FLATCFGFACTORY_HPP_INCLUDED
#define FLATCFGFACTORY_HPP_INCLUDED

#include <memory>

#include <string>
#include <vector>
#include "score/mw/launch_manager/alive_monitor/details/common/Types.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/IPhmFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/MachineConfigFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessStateReader.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"

namespace score {
    namespace lcm {
        class ControlClient;
    }
}

namespace score
{
namespace lcm
{
namespace saf
{

namespace logging
{
class PhmLogger;
}

namespace factory
{

/// @brief PHM Factory that reads configuration from the unified launch_manager_config.bin
/// @details Provides methods to create worker objects from the new Config type
///          and establishes required links between the worker objects automatically.
class FlatCfgFactory : public IPhmFactory
{
public:
    /// @brief Constructor
    /// @param [in] f_bufferConfig_r Buffer configuration used for constructing supervisions
    explicit FlatCfgFactory(const factory::MachineConfigFactory::SupervisionBufferConfig& f_bufferConfig_r);

    /// @brief Destructor
    ~FlatCfgFactory() override = default;

    /// @brief No Copy Constructor
    FlatCfgFactory(const FlatCfgFactory&) = delete;
    /// @brief No Copy Assignment
    FlatCfgFactory& operator=(const FlatCfgFactory&) = delete;
    /// @brief No Move Constructor
    FlatCfgFactory(FlatCfgFactory&&) = delete;
    /// @brief No Move Assignment
    FlatCfgFactory& operator=(FlatCfgFactory&&) = delete;

    /// @brief Initialize from the unified Config object
    /// @param [in] config  The parsed launch manager configuration
    /// @return             true on success
    bool init(const score::mw::launch_manager::configuration::Config& config);

    /// @brief Refer to the description of the base class (IPhmFactory)
    bool createProcessStates(std::vector<ifexm::ProcessState>& f_processStates_r,
                             ifexm::ProcessStateReader& f_processStateReader_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createMonitorIfIpcs(std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createMonitorIf(std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
                                  std::vector<ifappl::CheckpointIpcServer>& f_interfaceIpcs_r,
                                  std::vector<ifexm::ProcessState>& f_processStates_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createSupervisionCheckpoints(std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                      std::vector<ifappl::MonitorIfDaemon>& f_interfaces_r,
                                      std::vector<ifexm::ProcessState>& f_processStates_r) override;

    /// Refer to the description of the base class (IPhmFactory)
    bool createAliveSupervisions(std::vector<supervision::Alive>& f_alive_r,
                                 std::vector<ifappl::Checkpoint>& f_checkpoints_r,
                                 std::vector<ifexm::ProcessState>& f_processStates_r,
                                 std::shared_ptr<score::lcm::IRecoveryClient> f_recoveryClient_r) override;

private:

    /// @brief Get process id from a component configuration
    /// @param[in] comp  Pointer to the component configuration
    /// @return          process id or nullopt in case of an error
    std::optional<common::ProcessId> getProcessId(
        const score::mw::launch_manager::configuration::ComponentConfig* comp) noexcept(true);

    /// @brief Create IPC Channel with uid-based access permission
    bool initIpcServerWithUidBasedAccess(ifappl::CheckpointIpcServer& f_ipcServer_r,
                                         const std::string& f_ipcPath_r,
                                         const std::int32_t f_uid) noexcept(false);

    /// @brief The buffer configuration for constructing supervision objects
    const factory::MachineConfigFactory::SupervisionBufferConfig& bufferConfig_r;

    /// Non-owning pointer to the unified configuration. Must outlive this factory —
    /// typically the Config lives in main() and the factory is destroyed before it.
    const score::mw::launch_manager::configuration::Config* config_;

    /// Filtered list of supervised components (pointers into config_->components())
    std::vector<const score::mw::launch_manager::configuration::ComponentConfig*> supervised_components_;

    /// Stable storage for alive supervision config names, so that raw const char* pointers
    /// passed via AliveSupervisionCfg::cfgName_p remain valid for the factory's lifetime.
    std::vector<std::string> alive_cfg_names_;

    /// Logger object for logging messages
    logging::PhmLogger& logger_r;
};

}  // namespace factory
}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif
