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
#include "health_monitor_thread.hpp"

namespace score
{
namespace lcm
{
namespace internal 
{

HealthMonitorThread::HealthMonitorThread(std::unique_ptr<saf::daemon::IHealthMonitor> health_monitor) : m_health_monitor(std::move(health_monitor)) {

}

bool HealthMonitorThread::start() {
    score::lcm::saf::daemon::EInitCode init_status{score::lcm::saf::daemon::EInitCode::kNotInitialized};
    health_monitor_thread_ = std::thread([this, &init_status]() {
        const auto initResult = m_health_monitor->init();
        
        notifyInitializationComplete(init_status, initResult);

        if (initResult == saf::daemon::EInitCode::kNoError) {
            m_health_monitor->run(stop_thread_);
        }
    });

    waitForInitializationCompleted(init_status);

    return (init_status == score::lcm::saf::daemon::EInitCode::kNoError);
}

void HealthMonitorThread::stop() {
    stop_thread_.store(true);
    if (health_monitor_thread_.joinable()) {
        health_monitor_thread_.join();
    }
}

void HealthMonitorThread::notifyInitializationComplete(
    score::lcm::saf::daemon::EInitCode& f_init_status_r,
    const score::lcm::saf::daemon::EInitCode f_init_result) {
    {
        std::lock_guard lk(m_initialization_mutex);
        f_init_status_r = f_init_result;
    }
    m_initialization_cv.notify_all();
}

void HealthMonitorThread::waitForInitializationCompleted(
    score::lcm::saf::daemon::EInitCode& f_init_status_r) {
    std::unique_lock lk(m_initialization_mutex);
    m_initialization_cv.wait(
        lk,
        [&f_init_status_r]() {
            return f_init_status_r != score::lcm::saf::daemon::EInitCode::kNotInitialized;
        });
}

}
}
}
