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

#include "score/mw/launch_manager/alive_monitor/details/daemon/PhmDaemon.hpp"

#include "score/mw/launch_manager/alive_monitor/details/factory/FlatCfgFactory.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/MonitorIfDaemon.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace daemon
{

/* RULECHECKER_comment(0, 6, check_expensive_to_copy_in_parameter, "Move only types cannot be passed by const ref",
   true_no_defect) */
/* RULECHECKER_comment(0, 4, check_incomplete_data_member_construction, "Default constructor is used for\
 processStateReader.", true_no_defect) */
PhmDaemon::PhmDaemon(
    OsClock& f_osClock,
    std::unique_ptr<Watchdog> f_watchdog,
    std::unique_ptr<ProcessStateReceiver> f_process_state_receiver)
    : osClock{f_osClock},
      cycleTimer{&osClock},
      swClusterHandlers{},
      processStateReader{std::move(f_process_state_receiver)},
      watchdog(std::move(f_watchdog))
{
    static_cast<void>(f_osClock);
}

void PhmDaemon::performCyclicTriggers(void)
{
    bool isCriticalFailure{false};

    NanoSecondType syncTimestamp{timers::OsClock::getMonotonicSystemClock()};
    if (syncTimestamp == 0U)
    {
        // No valid time value, use max value for synchronization
        // All received data will be considered.
        syncTimestamp = UINT64_MAX;
    }

    isCriticalFailure = (!processStateReader.distributeChanges(syncTimestamp));

    if (!isCriticalFailure)
    {
        for (auto& phmHandler : swClusterHandlers)
        {
            phmHandler.performCyclicTriggers(syncTimestamp);
            isCriticalFailure = isCriticalFailure || phmHandler.hasAnyRecoveryEnqueueFailed();
        }
    }

    // watchdog is fired iff:
    //  * isCriticalFailure is set (e.g. process state distribution error, recovery ring buffer full)
    // else:
    //  * watchdog is serviced
    if (!isCriticalFailure)
    {
        watchdog->serviceWatchdog();
    }
    else
    {
        watchdog->fireWatchdogReaction();
    }
}

#ifdef USE_NEW_CONFIGURATION
bool PhmDaemon::construct(const Config& config, const SupervisionBufferConfig& f_bufferConfig_r) noexcept(false)
#else
bool PhmDaemon::construct(const SupervisionBufferConfig& f_bufferConfig_r) noexcept(false)
#endif
{
    bool isSuccess{true};

    score::Result<std::vector<std::string>> listSwClustersPhm{{"MainCluster"}};
    if (!listSwClustersPhm.has_value())
    {
        LM_LOG_ERROR() << "Phm Daemon: retrieving the list of PHM software cluster configurations failed with error:"
                       << listSwClustersPhm.error().Message();
        isSuccess = false;
    }
    else
    {
        if (listSwClustersPhm.value().size() == 0U)
        {
            LM_LOG_WARN() << "Phm Daemon: is starting without any software cluster configurations!";
        }

        // Reserve the vector swClusterHandlers obtained from flatcfg before constructing the SwClusters
        swClusterHandlers.reserve(listSwClustersPhm.value().size());

        for (auto strSwClusterName : listSwClustersPhm.value())
        {
            swClusterHandlers.emplace_back(strSwClusterName);
#ifdef USE_NEW_CONFIGURATION
            isSuccess =
                swClusterHandlers.back().constructWorkers(config, recoveryClient, processStateReader, f_bufferConfig_r);
#else
            isSuccess = swClusterHandlers.back().constructWorkers(recoveryClient, processStateReader, f_bufferConfig_r);
#endif
            if (!isSuccess)
            {
                LM_LOG_ERROR() << "Phm Daemon: failed to create worker objects for swclusterhandler:"
                               << strSwClusterName;
                break;
            }
        }
    }
    return isSuccess;
}

}  // namespace daemon
}  // namespace saf
}  // namespace lcm
}  // namespace score
