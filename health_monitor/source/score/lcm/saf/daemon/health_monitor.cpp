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

#include "score/lcm/saf/common/PhmSignalHandler.hpp"
#include "score/lcm/saf/daemon/PhmDaemon.hpp"
#include "score/lcm/saf/logging/PhmLogger.hpp"
#include "score/lcm/saf/watchdog/WatchdogImpl.hpp"

#ifdef BINARY_TEST_ENABLE_PHM_DAEMON_HEAP_MEASUREMENT
#    include "tracer.hpp"
#endif

/// @file rb-phmd.cpp
/// @brief Main function and thus entry point of the PHM daemon
/// @param[in] argc Number of command line arguments (first argument is the executable name itself)
/// @param[in] argv Array pointing to the command line arguments (first argument is the executable name itself)
/// @return execution status of PHM daemon to the operating system
/// - 0 on successful execution
/// - EXIT_FAILURE (1) if initialization could not be performed due to an error
/// @details
/// 1. Register signal handling
/// 2. Set up the logger and a PhmDaemon object
/// 3. Initialize the PhmDaemon object
/// 4. Enters the cyclic loop if initialization was successful (cyclic loop can be terminated by a signal received)
#ifdef UNITTEST_MAIN_TO_FUNCTION
int rb_phmd_main(int argc, char** argv)
#else
int main(int argc, char** argv)
#endif
{
    int returnValue{EXIT_SUCCESS};

    // Overall try-catch block for exception handling
    try
    {
        if (!score::lcm::saf::common::PhmSignalHandler::ignoreTerminationSignals())
        {
            return EXIT_FAILURE;
        }

        score::lcm::saf::timers::OsClockInterface osClock{};
        osClock.startMeasurement();

        score::lcm::saf::logging::PhmLogger& logger_r{
            score::lcm::saf::logging::PhmLogger::getLogger(score::lcm::saf::logging::PhmLogger::EContext::factory)};

        std::unique_ptr<score::lcm::saf::watchdog::IWatchdogIf> watchdog{};
        watchdog = std::make_unique<score::lcm::saf::watchdog::WatchdogImpl>();
        score::lcm::saf::daemon::PhmDaemon daemon{osClock, logger_r, std::move(watchdog)};

        // coverity[autosar_cpp14_a15_5_2_violation] This warning comes from pipc-sa(external library)
        const score::lcm::saf::daemon::PhmDaemon::EInitCode initResult{daemon.init(argc, argv)};

        if (score::lcm::saf::daemon::PhmDaemon::EInitCode::kNoError == initResult)
        {
            const long ms{osClock.endMeasurement()};
            logger_r.LogDebug() << "Phm Daemon: Initialization took " << ms << " ms";
            score::lcm::LifecycleClient client{};

#ifdef BINARY_TEST_ENABLE_PHM_DAEMON_HEAP_MEASUREMENT
            score::lcm::heap::Tracer::Enable();
#endif

            const bool isRunning{
                daemon.startCyclicExec(client, score::lcm::saf::common::PhmSignalHandler::receivedTerminationSignal)};

            if (!isRunning)
            {
                logger_r.LogError() << "Phm Daemon: Start of cyclic execution failed.";
                returnValue = EXIT_FAILURE;
            }
        }
        else if (score::lcm::saf::daemon::PhmDaemon::EInitCode::kPrintHelpOrVersion == initResult)
        {
            logger_r.LogInfo() << "Phm Daemon: will exit.";
        }
        else
        {
            logger_r.LogError() << "Phm Daemon: Initialization failed!";
            returnValue = EXIT_FAILURE;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Phm Daemon: Initialization failed due to standard exception: " << e.what() << ".\n";
        returnValue = EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "Phm Daemon: Initialization failed due to exception!\n";
        returnValue = EXIT_FAILURE;
    }

    return returnValue;
}
