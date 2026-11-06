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
#include <iostream>

#include <score/assert.hpp>

#include "score/mw/launch_manager/alive_monitor/details/daemon/AliveMonitorImpl.hpp"
#include "score/mw/launch_manager/alive_monitor/details/daemon/CyclicExecutor.hpp"
#include "score/os/utils/thread.h"

namespace score::mw::lifecycle::internal::saf::daemon
{

AliveMonitorImpl::AliveMonitorImpl(
    SptrIRecoveryClient recovery_client,
    AliveSupervisionConfig config,
    const std::size_t supervised_components)
    : recovery_client_(recovery_client), config_(config), supervised_components_(supervised_components)
{
}

bool AliveMonitorImpl::init() noexcept
{
    try
    {
        os_clock_.startMeasurement();

        daemon_ = std::make_unique<CyclicExecutor>(os_clock_, supervised_components_);
        EInitCode initResult = daemon_->init(recovery_client_, config_);

        if (initResult == EInitCode::kNoError)
        {
            const long ms{os_clock_.endMeasurement()};
            LM_LOG_DEBUG() << "AliveMonitor: Initialization took" << ms << "ms";
            return true;
        }
        else
        {
            LM_LOG_ERROR() << "AliveMonitor: Initialization failed with error code:" << static_cast<int>(initResult);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "AliveMonitor: Initialization failed due to standard exception: " << e.what() << ".\n";
    }
    catch (...)
    {
        std::cerr << "AliveMonitor: Initialization failed due to exception!\n";
    }

    return false;
}

void AliveMonitorImpl::startMonitoring() noexcept
{
    alive_monitor_thread_ = std::thread([this]() {
        threadFn(stop_thread_);
    });
    score::os::set_thread_name(alive_monitor_thread_, "health_mon");
}

void AliveMonitorImpl::stopMonitoring() noexcept
{
    stop_thread_.store(true);
    if (alive_monitor_thread_.joinable())
    {
        alive_monitor_thread_.join();
    }
}

bool AliveMonitorImpl::threadFn(std::atomic_bool& cancel_thread) noexcept
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(
        daemon_ != nullptr, "HealthMonitor: Instance is not initialized!");
    return daemon_->startCyclicExec(cancel_thread);
}

ISupervisionFactory& AliveMonitorImpl::getSupervisionFactory() const noexcept
{
    return *daemon_;
}

}  // namespace score::mw::lifecycle::internal::saf::daemon
