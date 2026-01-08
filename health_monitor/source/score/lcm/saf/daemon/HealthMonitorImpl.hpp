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

#ifndef SAF_DAEMON_HEALTH_MONITOR_IMPL_HPP_INCLUDED
#define SAF_DAEMON_HEALTH_MONITOR_IMPL_HPP_INCLUDED

#include <memory>
#include <atomic>

#include "score/lcm/saf/daemon/IHealthMonitor.hpp"

namespace score {
namespace lcm {

class IRecoveryClient;

namespace saf {

namespace watchdog {
class IWatchdogIf;
}

namespace daemon {

class HealthMonitorImpl : public IHealthMonitor {
   public:
    HealthMonitorImpl(std::shared_ptr<score::lcm::IRecoveryClient> recovery_client, std::unique_ptr<watchdog::IWatchdogIf> watchdog);

    EInitCode init() noexcept override;

    bool run(std::atomic_bool& cancel_thread) noexcept override;

   private:
    std::shared_ptr<score::lcm::IRecoveryClient> m_recovery_client{nullptr};
    std::unique_ptr<score::lcm::saf::watchdog::IWatchdogIf> m_watchdog{nullptr};
    score::lcm::saf::logging::PhmLogger& m_logger;
    std::unique_ptr<score::lcm::saf::daemon::PhmDaemon> m_daemon{nullptr};
    score::lcm::saf::timers::OsClockInterface m_osClock{};
};

}  // namespace daemon
}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif