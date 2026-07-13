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
#ifndef SAF_DAEMON_ALIVE_MONITOR_IMPL_HPP_INCLUDED
#define SAF_DAEMON_ALIVE_MONITOR_IMPL_HPP_INCLUDED

#include <memory>
#include <atomic>

#include "score/mw/launch_manager/alive_monitor/details/daemon/IAliveMonitor.hpp"
#ifdef USE_NEW_CONFIGURATION
#include "score/mw/launch_manager/configuration/config.hpp"
#endif

namespace score {
namespace lcm {

class IRecoveryClient;

namespace saf {

namespace watchdog {
class IWatchdogIf;
}

namespace daemon {

using SptrIRecoveryClient = std::shared_ptr<score::lcm::IRecoveryClient>;
using UptrIWatchdogIf = std::unique_ptr<watchdog::IWatchdogIf>;
using UptrIProcessStateReceiver = std::unique_ptr<score::lcm::IProcessStateReceiver>;
using Logger = score::lcm::saf::logging::PhmLogger;
using UptrPhmDaemon = std::unique_ptr<score::lcm::saf::daemon::PhmDaemon>;
using OsClock = score::lcm::saf::timers::OsClockInterface;
#ifdef USE_NEW_CONFIGURATION
using Config = score::mw::launch_manager::configuration::Config;
#endif

class AliveMonitorImpl : public IAliveMonitor {
   public:
#ifdef USE_NEW_CONFIGURATION
    AliveMonitorImpl(SptrIRecoveryClient recovery_client,
                     UptrIWatchdogIf watchdog,
                     UptrIProcessStateReceiver process_state_receiver,
                     const Config& config);
#else
    AliveMonitorImpl(SptrIRecoveryClient recovery_client, 
                     UptrIWatchdogIf watchdog, 
                     UptrIProcessStateReceiver process_state_receiver);
#endif

    EInitCode init() noexcept override;

    bool run(std::atomic_bool& cancel_thread) noexcept override;

   private:
    SptrIRecoveryClient m_recovery_client{nullptr};
    UptrIWatchdogIf m_watchdog{nullptr};
    Logger& m_logger;
    UptrPhmDaemon m_daemon{nullptr};
    OsClock m_osClock{};
    UptrIProcessStateReceiver m_process_state_receiver;
#ifdef USE_NEW_CONFIGURATION
    const Config& m_config;
#endif
};

}  // namespace daemon
}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif
