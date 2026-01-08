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


#ifndef SCORE_LCM_HEALTH_MONITOR_HPP_INCLUDED
#define SCORE_LCM_HEALTH_MONITOR_HPP_INCLUDED

#include <thread>
#include <atomic>
#include <score/lcm/internal/ihealth_monitor_thread.hpp>
#include <score/lcm/saf/daemon/IHealthMonitor.hpp>

namespace score
{
namespace lcm
{
namespace internal
{

/// @brief HealthMonitor manages the lifecycle of the Health Monitor daemon in a separate thread.
class HealthMonitorThread  final : public IHealthMonitorThread {
 public:
   HealthMonitorThread(std::unique_ptr<saf::daemon::IHealthMonitor> health_monitor);

    /// @brief Starts the Health Monitor thread.
    /// @return true if the Health Monitor started successfully, false otherwise.
    bool start() override;

    /// @brief Stops the Health Monitor thread.
    void stop() override;
 private:
   void notifyInitializationComplete(
      score::lcm::saf::daemon::EInitCode& f_init_status_r,
      const score::lcm::saf::daemon::EInitCode f_init_result);
   void waitForInitializationCompleted(score::lcm::saf::daemon::EInitCode& f_init_status_r);

    std::unique_ptr<saf::daemon::IHealthMonitor> m_health_monitor{nullptr};
    std::thread health_monitor_thread_{};
    std::atomic_bool stop_thread_{false};
    std::mutex m_initialization_mutex{};
    std::condition_variable m_initialization_cv{};
};
    
} 
}
}
#endif