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
#include <score/lcm/internal/health_monitor_thread.hpp>

namespace score
{
namespace lcm
{
namespace internal 
{

namespace {
    void notifyInitializationComplete(
        std::mutex& f_initialization_mutex_r,
        std::condition_variable& f_initialization_cv_r,
        score::lcm::saf::daemon::EInitCode& f_init_status_r,
        const score::lcm::saf::daemon::EInitCode f_init_result) noexcept {
        {
            std::lock_guard lk(f_initialization_mutex_r);
            f_init_status_r = f_init_result;
        }
        f_initialization_cv_r.notify_all();
    }

    void waitForInitializationCompleted(
        std::mutex& f_initialization_mutex_r,
        std::condition_variable& f_initialization_cv_r,
        score::lcm::saf::daemon::EInitCode& f_init_status_r) noexcept {
        std::unique_lock lk(f_initialization_mutex_r);
        f_initialization_cv_r.wait(
            lk,
            [&f_init_status_r]() {
                return f_init_status_r != score::lcm::saf::daemon::EInitCode::kNotInitialized;
            });
    }
}

HealthMonitorThread::HealthMonitorThread(std::unique_ptr<saf::daemon::IHealthMonitor> health_monitor) : m_health_monitor(std::move(health_monitor)) {

}

bool HealthMonitorThread::start() noexcept {
    score::lcm::saf::daemon::EInitCode init_status{score::lcm::saf::daemon::EInitCode::kNotInitialized};
    std::mutex initialization_mutex;
    std::condition_variable initialization_cv;

    health_monitor_thread_ = std::thread([this, &initialization_mutex, &init_status, &initialization_cv]() {
        const auto initResult = m_health_monitor->init();
        
        notifyInitializationComplete(
            initialization_mutex,
            initialization_cv,
            init_status,
            initResult);

        if (initResult == saf::daemon::EInitCode::kNoError) {
            m_health_monitor->run(stop_thread_);
        }
    });

    waitForInitializationCompleted(
        initialization_mutex,
        initialization_cv,
        init_status);

    return (init_status == score::lcm::saf::daemon::EInitCode::kNoError);
}

void HealthMonitorThread::stop() noexcept {
    stop_thread_.store(true);
    if (health_monitor_thread_.joinable()) {
        health_monitor_thread_.join();
    }
}

}
}
}
