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

#include "score/mw/lifecycle/alive.h"
#include "score/mw/launch_manager/alive_monitor/details/AliveImpl.h"

#include <utility>

// The public API is only sending alive notification. No need to support different checkpoints.
static constexpr std::uint32_t kDefaultCheckpointId{1U};

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

void Alive::ReportAlive() const noexcept
{
    if (aliveImplPtr.get() != nullptr)
    {
        aliveImplPtr->ReportCheckpoint(kDefaultCheckpointId);
    }
}

}  // namespace score::mw::lifecycle


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

void score_lcm_alive_report_alive(void* instance) noexcept {
    static_cast<score::mw::lifecycle::Alive*>(instance)->ReportAlive();
}

#ifdef __cplusplus
}
#endif
