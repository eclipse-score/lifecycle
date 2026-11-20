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

#include "score/mw/launch_manager/alive_monitor/details/daemon/SupervisionManager.hpp"
#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/MonitorIfDaemon.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"

namespace score::mw::lifecycle::internal::saf::daemon
{

SupervisionManager::SupervisionManager(std::unique_ptr<factory::IAliveWorkerFactory> factory)
    : process_states_{},
      alive_if_ipcs_{},
      alive_interfaces_{},
      alive_supervisions_{},
      flat_cfg_factory_{std::move(factory)}
{
}

SupervisionManager::~SupervisionManager() = default;

bool SupervisionManager::full() const
{
    return alive_supervisions_.size() >= capacity_;
}

void SupervisionManager::reserve(std::size_t size)
{
    capacity_ = size;
    process_states_.reserve(size);
    alive_if_ipcs_.reserve(size);
    alive_interfaces_.reserve(size);
    alive_supervisions_.reserve(size);
}

bool SupervisionManager::constructWorker(
    const IdentifierHash& id,
    const ComponentAliveSupervision& component_config,
    const uid_t uid,
    std::shared_ptr<mw::lifecycle::IRecoveryClient> f_recoveryClient_r,
    ifexm::ObservableEventReader& f_processStateReader_r) noexcept(false)
{
    if (!flat_cfg_factory_->createObservableEvent(process_states_, id, f_processStateReader_r))
    {
        return false;
    }
    if (!flat_cfg_factory_->createAliveIfIpc(alive_if_ipcs_, id, uid))
    {
        return false;
    }
    if (!flat_cfg_factory_->createAliveIf(alive_interfaces_, alive_if_ipcs_.back(), process_states_.back()))
    {
        return false;
    }
    if (!flat_cfg_factory_->createAliveSupervision(
            alive_supervisions_,
            alive_interfaces_.back(),
            process_states_.back(),
            f_recoveryClient_r,
            id,
            component_config))
    {
        return false;
    }

    return true;
}

void SupervisionManager::checkInterfaceForNewData(const std::chrono::nanoseconds f_syncTimestamp)
{
    for (auto& aliveInterface : alive_interfaces_)
    {
        aliveInterface.checkForNewData(f_syncTimestamp);
    }
}

void SupervisionManager::evaluateSupervisions(const std::chrono::nanoseconds f_syncTimestamp)
{
    for (auto& alive : alive_supervisions_)
    {
        alive.evaluate(f_syncTimestamp);
    }
}

bool SupervisionManager::hasAnyRecoveryEnqueueFailed() const noexcept
{
    for (const auto& alive : alive_supervisions_)
    {
        if (alive.hasRecoveryEnqueueFailed())
        {
            return true;
        }
    }
    return false;
}

void SupervisionManager::performCyclicTriggers(const std::chrono::nanoseconds f_syncTimestamp)
{
    checkInterfaceForNewData(f_syncTimestamp);
    evaluateSupervisions(f_syncTimestamp);
}

}  // namespace score::mw::lifecycle::internal::saf::daemon
