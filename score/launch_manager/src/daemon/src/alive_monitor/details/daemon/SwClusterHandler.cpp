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

#include "score/mw/launch_manager/alive_monitor/details/daemon/SwClusterHandler.hpp"

#include "score/mw/launch_manager/alive_monitor/details/factory/FlatCfgFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/MonitorIfDaemon.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace daemon
{

SwClusterHandler::SwClusterHandler(const std::string& f_swClusterName_r)
    : logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::factory)),
      f_swClusterName(f_swClusterName_r),
      processStates{},
      aliveIfIpcs{},
      aliveInterfaces{},
      checkpoints{},
      aliveSupervisions{}
{
    if (f_swClusterName_r.empty())
    {
        logger_r.LogError() << "Software Cluster Handler: Software cluster name is empty!";
    }
}

SwClusterHandler::~SwClusterHandler() = default;

bool SwClusterHandler::constructWorkers(
    const score::mw::launch_manager::configuration::Config& config,
    std::shared_ptr<score::lcm::IRecoveryClient> f_recoveryClient_r,
    ifexm::ProcessStateReader& f_processStateReader_r,
    const factory::MachineConfigFactory::SupervisionBufferConfig& f_bufferConfig_r) noexcept(false)
{
    bool isSuccess{false};
    factory::FlatCfgFactory flatCfgFactory{f_bufferConfig_r};

    isSuccess = flatCfgFactory.init(config);
    if (isSuccess)
    {
        logger_r.LogDebug() << "Software Cluster Handler starts constructing workers:" << f_swClusterName;
        isSuccess = flatCfgFactory.createProcessStates(processStates, f_processStateReader_r);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createAliveIfIpcs(aliveIfIpcs);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createAliveIf(aliveInterfaces, aliveIfIpcs, processStates);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createSupervisionCheckpoints(checkpoints, aliveInterfaces, processStates);
    }
    if (isSuccess)
    {
        isSuccess =
            flatCfgFactory.createAliveSupervisions(aliveSupervisions, checkpoints, processStates, f_recoveryClient_r);
    }
    if (isSuccess == false)
    {
        logger_r.LogError() << "Software Cluster Handler is unable to construct the required worker objects.";
    }
    return isSuccess;
}

void SwClusterHandler::checkInterfaceForNewData(const timers::NanoSecondType f_syncTimestamp)
{
    for (auto& aliveInterface : aliveInterfaces)
    {
        aliveInterface.checkForNewData(f_syncTimestamp);
    }
}

void SwClusterHandler::evaluateSupervisions(const timers::NanoSecondType f_syncTimestamp)
{
    for (auto& alive : aliveSupervisions)
    {
        alive.evaluate(f_syncTimestamp);
    }
}

bool SwClusterHandler::hasAnyRecoveryEnqueueFailed() const noexcept
{
    for (const auto& alive : aliveSupervisions)
    {
        if (alive.hasRecoveryEnqueueFailed())
        {
            return true;
        }
    }
    return false;
}

void SwClusterHandler::performCyclicTriggers(const timers::NanoSecondType f_syncTimestamp)
{
    checkInterfaceForNewData(f_syncTimestamp);
    evaluateSupervisions(f_syncTimestamp);
}

}  // namespace daemon
}  // namespace saf
}  // namespace lcm
}  // namespace score
