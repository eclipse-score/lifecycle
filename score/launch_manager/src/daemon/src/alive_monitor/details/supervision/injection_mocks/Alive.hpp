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

#ifndef ALIVE_HPP_INCLUDED
#define ALIVE_HPP_INCLUDED

#include <gmock/gmock.h>
#include <cstdint>

#include "score/mw/launch_manager/alive_monitor/details/common/Observer.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/ISupervision.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/recovery_client/irecovery_client.h"

namespace score::mw::lifecycle::internal::saf::supervision
{

using configuration::ComponentAliveSupervision;

enum class EMockStatus : uint32_t
{
    kOk = 0U,
    kFailed = 1U,
    kExpired = 2U,
    kDeactivated = 4U
};

class MockAlive
{
  public:
    using EStatus = EMockStatus;

    MOCK_METHOD(
        void,
        ctor,
        (const IdentifierHash id,
         const ComponentAliveSupervision& f_aliveCfg_r,
         const std::shared_ptr<IRecoveryClient> recovery_client,
         common::Observable<ifappl::Checkpoint>& interface,
         const uint16_t bufferSize),
        (noexcept(false)));
    MOCK_METHOD(void, updateData, (const ifappl::Checkpoint& f_observable_r), (noexcept(true)));
    MOCK_METHOD(void, updateData, (const ifexm::ObservableEvent& f_observable_r), (noexcept(true)));
    MOCK_METHOD(void, evaluate, (const std::chrono::nanoseconds f_syncTimestamp), ());
    MOCK_METHOD(EStatus, getStatus, (), (const, noexcept(true)));
    MOCK_METHOD(std::chrono::nanoseconds, getTimestamp, (), (const, noexcept(true)));
    MOCK_METHOD(bool, hasRecoveryEnqueueFailed, (), (const, noexcept));
    MOCK_METHOD(IdentifierHash, getIdentifier, (), (const, noexcept));
};

extern std::unique_ptr<MockAlive> g_mock_alive_supervision;

class Alive : public ISupervision,
              public saf::common::Observable<Alive>,
              public saf::common::Observer<ifappl::Checkpoint>,
              public saf::common::Observer<ifexm::ObservableEvent>
{
  public:
    using EStatus = EMockStatus;

    Alive() = delete;
    Alive(Alive&&) = default;
    Alive& operator=(Alive&&) = delete;
    Alive(const Alive&) = delete;
    Alive& operator=(const Alive&) = delete;
    ~Alive() override = default;

    explicit Alive(
        const IdentifierHash id,
        const ComponentAliveSupervision& f_aliveCfg_r,
        const std::shared_ptr<IRecoveryClient> recovery_client,
        common::Observable<ifappl::Checkpoint>& interface,
        const uint16_t bufferSize) noexcept(false)
        : ISupervision(id)
    {
        if (g_mock_alive_supervision)
        {
            g_mock_alive_supervision->ctor(id, f_aliveCfg_r, recovery_client, interface, bufferSize);
        }
    }

    void updateData(const ifappl::Checkpoint& f_observable_r) noexcept(true) override
    {
        g_mock_alive_supervision->updateData(f_observable_r);
    }

    void updateData(const ifexm::ObservableEvent& f_observable_r) noexcept(true) override
    {
        g_mock_alive_supervision->updateData(f_observable_r);
    }

    void evaluate(const std::chrono::nanoseconds f_syncTimestamp) override
    {
        g_mock_alive_supervision->evaluate(f_syncTimestamp);
    }

    EStatus getStatus(void) const noexcept(true)
    {
        return g_mock_alive_supervision->getStatus();
    }

    std::chrono::nanoseconds getTimestamp(void) const noexcept(true)
    {
        return g_mock_alive_supervision->getTimestamp();
    }

    bool hasRecoveryEnqueueFailed(void) const noexcept
    {
        return g_mock_alive_supervision->hasRecoveryEnqueueFailed();
    }

    IdentifierHash getIdentifier() const noexcept override
    {
        return g_mock_alive_supervision->getIdentifier();
    }
};

}  // namespace score::mw::lifecycle::internal::saf::supervision

#endif
