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

#include "score/lcm/saf/daemon/SwClusterHandler.hpp"

#include "score/lcm/saf/factory/FlatCfgFactory.hpp"
#include "score/lcm/saf/ifappl/Checkpoint.hpp"
#include "score/lcm/saf/ifappl/MonitorIfDaemon.hpp"
#include "score/lcm/saf/supervision/Alive.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace daemon
{

SwClusterHandler::SwClusterHandler(const std::string& f_swClusterName_r) :
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::factory)),
    f_swClusterName(f_swClusterName_r),
    processStates{},
    monitorIfIpcs{},
    monitorInterfaces{},
    checkpoints{},
    aliveSupervisions{}
{
    if (f_swClusterName_r.empty())
    {
        logger_r.LogError() << "Software Cluster Handler: Software cluster name is empty!";
    }
}

SwClusterHandler::~SwClusterHandler() = default;

/* RULECHECKER_comment(0, 3, check_max_cyclomatic_complexity, "Max cyclomatic complexity violation\
   is tolerated for this function. ", true_no_defect) */
bool SwClusterHandler::constructWorkers(std::shared_ptr<score::lcm::IRecoveryClient> f_recoveryClient_r, ifexm::ProcessStateReader& f_processStateReader_r,
    const factory::MachineConfigFactory::SupervisionBufferConfig& f_bufferConfig_r) noexcept(false)
{
    bool isSuccess{false};
    factory::FlatCfgFactory flatCfgFactory{f_bufferConfig_r};

    const std::string filename = "hm_demo.bin";
    isSuccess = flatCfgFactory.init(filename);
    if (isSuccess)
    {
        logger_r.LogDebug() << "Software Cluster Handler starts constructing workers:" << f_swClusterName;
        isSuccess = flatCfgFactory.createProcessStates(processStates, f_processStateReader_r);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createMonitorIfIpcs(monitorIfIpcs);
    }
    if (isSuccess)
    {
        isSuccess =
            flatCfgFactory.createMonitorIf(monitorInterfaces, monitorIfIpcs, processStates);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createSupervisionCheckpoints(checkpoints, monitorInterfaces, processStates);
    }
    if (isSuccess)
    {
        isSuccess = flatCfgFactory.createAliveSupervisions(aliveSupervisions, checkpoints, processStates, f_recoveryClient_r);
    }
    if (isSuccess == false)
    {
        logger_r.LogError() << "Software Cluster Handler is unable to construct the required worker objects.";
    }
    return isSuccess;
}

void SwClusterHandler::checkInterfaceForNewData(const timers::NanoSecondType f_syncTimestamp)
{
    for (auto& monitorInterface : monitorInterfaces)
    {
        monitorInterface.checkForNewData(f_syncTimestamp);
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
