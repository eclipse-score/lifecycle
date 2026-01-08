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

#include "score/lcm/saf/daemon/HealthMonitorImpl.hpp"
#include "score/lcm/saf/logging/PhmLogger.hpp"
#include "score/lcm/saf/watchdog/WatchdogImpl.hpp"

namespace score {
namespace lcm {
namespace saf {
namespace daemon {

HealthMonitorImpl::HealthMonitorImpl(std::shared_ptr<score::lcm::IRecoveryClient> recovery_client, std::unique_ptr<watchdog::IWatchdogIf> watchdog)
    : m_recovery_client(recovery_client), m_watchdog(std::move(watchdog)), m_logger{score::lcm::saf::logging::PhmLogger::getLogger(score::lcm::saf::logging::PhmLogger::EContext::factory)} {}

EInitCode HealthMonitorImpl::init() noexcept {
    score::lcm::saf::daemon::EInitCode initResult{score::lcm::saf::daemon::EInitCode::kGeneralError};
    try {
        m_osClock.startMeasurement();

        m_daemon = std::make_unique<score::lcm::saf::daemon::PhmDaemon>(m_osClock, m_logger, std::move(m_watchdog));
        initResult = m_daemon->init(m_recovery_client);

        if (initResult == score::lcm::saf::daemon::EInitCode::kNoError) {
            const long ms{m_osClock.endMeasurement()};
            m_logger.LogDebug() << "HealthMonitor: Initialization took " << ms << " ms";
        } else {
            m_logger.LogError() << "HealthMonitor: Initialization failed with error code:" << static_cast<int>(initResult);
        }
    } catch (const std::exception& e) {
        std::cerr << "HealthMonitor: Initialization failed due to standard exception: " << e.what() << ".\n";
        initResult = EInitCode::kGeneralError;
    } catch (...) {
        std::cerr << "HealthMonitor: Initialization failed due to exception!\n";
        initResult = EInitCode::kGeneralError;
    }

    return initResult;
}

bool HealthMonitorImpl::run(std::atomic_bool& cancel_thread) noexcept {
    assert(m_daemon != nullptr && "HealthMonitor: Instance is not initialized!");
    return m_daemon->startCyclicExec(cancel_thread);
}

}  // namespace daemon
}  // namespace saf
}  // namespace lcm
}  // namespace score