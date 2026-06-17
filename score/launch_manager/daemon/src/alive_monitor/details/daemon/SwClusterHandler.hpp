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

#ifndef SWCLUSTERHANDLER_HPP_INCLUDED
#define SWCLUSTERHANDLER_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/details/factory/MachineConfigFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/DataStructures.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessState.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessStateReader.hpp"
#include "score/mw/launch_manager/alive_monitor/details/logging/PhmLogger.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include <string>
#include <vector>

namespace score
{
namespace lcm
{

class IRecoveryClient;

namespace saf
{

// Forward declarations
namespace ifappl
{
class MonitorIfDaemon;
class Checkpoint;
}  // namespace ifappl

namespace supervision
{
class Alive;
}  // namespace supervision

// End Forward declarations

namespace daemon
{

/// @brief Software Cluster Handler wraps the full PHM Supervision and Recovery Notification functionality for one
///        Software Cluster.
class SwClusterHandler
{
  public:
    /// @brief No Default Constructor
    SwClusterHandler() = delete;

    /// @brief Constructor
    /// @param [in] f_swClusterName_r   Software Cluster name which shall be handled
    explicit SwClusterHandler(const std::string& f_swClusterName_r);

    /// @brief Destroys the workers
    virtual ~SwClusterHandler();

    /// @brief No Copy Constructor
    SwClusterHandler(const SwClusterHandler&) = delete;

    /// @brief No Copy Assignment
    SwClusterHandler& operator=(const SwClusterHandler&) = delete;

    /// @brief Move Constructor
    SwClusterHandler(SwClusterHandler&&) = default;

    /// @brief No Move Assignment
    SwClusterHandler& operator=(SwClusterHandler&&) = delete;

    /// @brief Construct required worker objects from the unified Config
    bool constructWorkers(
        const score::mw::launch_manager::configuration::Config& config,
        std::shared_ptr<score::lcm::IRecoveryClient> f_recoveryClient_r,
        ifexm::ProcessStateReader& f_processStateReader_r,
        const factory::MachineConfigFactory::SupervisionBufferConfig& f_bufferConfig_r) noexcept(false);

    /// @brief Perform cyclic execution
    void performCyclicTriggers(const timers::NanoSecondType f_syncTimestamp);

    /// @brief Check whether any alive supervision failed to enqueue a recovery request
    bool hasAnyRecoveryEnqueueFailed() const noexcept;

  private:
    void checkInterfaceForNewData(const timers::NanoSecondType f_syncTimestamp);
    void evaluateSupervisions(const timers::NanoSecondType f_syncTimestamp);

    /// @brief Logger
    logging::PhmLogger& logger_r;

    /// SwCluster Name for this SwCLusterHandler Object
    const std::string f_swClusterName;

    /// Vector of Process states
    std::vector<ifexm::ProcessState> processStates;

    /// Vector of Monitor Interface IPCs
    std::vector<ifappl::CheckpointIpcServer> monitorIfIpcs;

    /// Vector of Monitor Interfaces
    std::vector<ifappl::MonitorIfDaemon> monitorInterfaces;

    /// Vector of Supervision checkpoints
    std::vector<ifappl::Checkpoint> checkpoints;

    /// Vector of Alive Supervisions
    std::vector<supervision::Alive> aliveSupervisions;
};

}  // namespace daemon
}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif
