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

#include "score/mw/launch_manager/alive_monitor/Alive.h"
#include "score/mw/launch_manager/alive_monitor/details/MonitorImpl.h"

#include <utility>

namespace score::mw::lifecycle
{

Alive::Alive() noexcept(false) :
    monitorImplPtr(std::make_unique<MonitorImpl>("default_instance"))
{
}

Alive::Alive(Alive&& se) noexcept :
    monitorImplPtr(std::move(se.monitorImplPtr))
{
}

Alive& Alive::operator=(Alive&& se) noexcept
{
    if (this != &se)
    {
        monitorImplPtr.reset(nullptr);
        monitorImplPtr = std::move(se.monitorImplPtr);
    }

    return *this;
}

Alive::~Alive() noexcept = default;

void Alive::ReportCheckpoint(std::uint32_t checkpointId) const noexcept
{
    if (monitorImplPtr.get() != nullptr)
    {
        monitorImplPtr->ReportCheckpoint(checkpointId);
    }
}

}  // namespace score::mw::lifecycle

enum class Dummy : std::uint32_t {};

#ifdef __cplusplus
extern "C" {
#endif

void* score_lcm_monitor_initialize() noexcept {
    try {
        auto* monitorPtr = new score::mw::lifecycle::Alive();
        return static_cast<void*>(monitorPtr);
    } catch (...) {
        return nullptr;
    }
}

void score_lcm_monitor_deinitialize(void* instance) noexcept {
    auto* monitorPtr = static_cast<score::mw::lifecycle::Alive*>(instance);
    delete monitorPtr;
}

void score_lcm_monitor_report_checkpoint(void* instance, std::uint32_t checkpointId) noexcept {
    static_cast<score::mw::lifecycle::Alive*>(instance)->ReportCheckpoint(checkpointId);
}

#ifdef __cplusplus
}
#endif
