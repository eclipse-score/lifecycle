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
#include "score/mw/launch_manager/alive_monitor/details/AliveImpl.h"

#include <utility>

namespace score::mw::lifecycle
{

Alive::Alive(const std::string_view& instance) noexcept(false) :
    aliveImplPtr(std::make_unique<AliveImpl>(instance))
{
}

Alive::Alive(Alive&& se) noexcept :
    aliveImplPtr(std::move(se.aliveImplPtr))
{
}

Alive& Alive::operator=(Alive&& se) noexcept
{
    if (this != &se)
    {
        aliveImplPtr.reset(nullptr);
        aliveImplPtr = std::move(se.aliveImplPtr);
    }

    return *this;
}

Alive::~Alive() noexcept = default;

void Alive::ReportCheckpoint(std::uint32_t checkpointId) const noexcept
{
    if (aliveImplPtr.get() != nullptr)
    {
        aliveImplPtr->ReportCheckpoint(checkpointId);
    }
}

}  // namespace score::mw::lifecycle

enum class Dummy : std::uint32_t {};

#ifdef __cplusplus
extern "C" {
#endif

void* score_lcm_alive_initialize(const char* instanceSpecifier) noexcept {
    try {
        auto* alivePtr = new score::mw::lifecycle::Alive(instanceSpecifier);
        return static_cast<void*>(alivePtr);
    } catch (...) {
        return nullptr;
    }
}

void score_lcm_alive_deinitialize(void* instance) noexcept {
    auto* alivePtr = static_cast<score::mw::lifecycle::Alive*>(instance);
    delete alivePtr;
}

void score_lcm_alive_report_checkpoint(void* instance, std::uint32_t checkpointId) noexcept {
    static_cast<score::mw::lifecycle::Alive*>(instance)->ReportCheckpoint(checkpointId);
}

#ifdef __cplusplus
}
#endif
