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

#include <sys/types.h>

#include <cstdint>
#include <iostream>

#include "score/lcm/saf/logging/PhmLogger.hpp"
#include "score/lcm/saf/watchdog/WatchdogImpl.hpp"
#include "health_monitor.hpp"

#ifdef BINARY_TEST_ENABLE_PHM_DAEMON_HEAP_MEASUREMENT
#    include "tracer.hpp"
#endif

/// @file health_monitor.cpp
/// @brief Entry point for HM daemon thread
/// @param[in] recovery_client Shared pointer to recovery client
/// @param[inout] init_status Atomic variable to communicate result of initialization
/// @param[in] cancel_thread Atomic variable to check whether the loop should be exited
/// @details
/// 1. Set up the logger and a PhmDaemon object
/// 2. Initialize the PhmDaemon object
/// 3. Enters the cyclic loop if initialization was successful (cyclic loop can be terminated by a signal received)
namespace score
{
namespace lcm
{
namespace saf
{
namespace daemon
{
void run(std::shared_ptr<score::lcm::IRecoveryClient> recovery_client, std::atomic<EInitCode>& init_status, std::atomic_bool& cancel_thread)
{
    try
    {

        score::lcm::saf::timers::OsClockInterface osClock{};
        osClock.startMeasurement();

        score::lcm::saf::logging::PhmLogger& logger_r{
            score::lcm::saf::logging::PhmLogger::getLogger(score::lcm::saf::logging::PhmLogger::EContext::factory)};

        std::unique_ptr<score::lcm::saf::watchdog::IWatchdogIf> watchdog{};
        watchdog = std::make_unique<score::lcm::saf::watchdog::WatchdogImpl>();
        score::lcm::saf::daemon::PhmDaemon daemon{osClock, logger_r, std::move(watchdog)};

        // coverity[autosar_cpp14_a15_5_2_violation] This warning comes from pipc-sa(external library)
        const score::lcm::saf::daemon::EInitCode initResult{daemon.init(recovery_client)};

        if (score::lcm::saf::daemon::EInitCode::kNoError == initResult)
        {
            const long ms{osClock.endMeasurement()};
            logger_r.LogDebug() << "Phm Daemon: Initialization took " << ms << " ms";

#ifdef BINARY_TEST_ENABLE_PHM_DAEMON_HEAP_MEASUREMENT
            score::lcm::heap::Tracer::Enable();
#endif

            const bool isRunning{
                daemon.startCyclicExec(cancel_thread, init_status)};

            if (!isRunning)
            {
                logger_r.LogError() << "Phm Daemon: Start of cyclic execution failed.";
                {
                    std::lock_guard lk(initialization_mutex);
                    init_status.store(EInitCode::kGeneralError);
                }
                initialization_cv.notify_all();
            }
        }
        else
        {
            logger_r.LogError() << "Phm Daemon: Initialization failed!";
            {
                std::lock_guard lk(initialization_mutex);
                init_status.store(initResult);
            }
            initialization_cv.notify_all();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Phm Daemon: Initialization failed due to standard exception: " << e.what() << ".\n";
        init_status.store(EInitCode::kGeneralError);
        {
            std::lock_guard lk(initialization_mutex);
            init_status.store(EInitCode::kGeneralError);
        }
        initialization_cv.notify_all();
    }
    catch (...)
    {
        std::cerr << "Phm Daemon: Initialization failed due to exception!\n";
        {
            std::lock_guard lk(initialization_mutex);
            init_status.store(EInitCode::kGeneralError);
        }
        initialization_cv.notify_all();
    }
}
}  // namespace daemon
}  // namespace saf
}  // namespace lcm
}  // namespace score