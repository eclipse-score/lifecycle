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

#ifndef PROCESS_MONITOR_HPP_INCLUDED
#define PROCESS_MONITOR_HPP_INCLUDED

#include "score/mw/launch_manager/process_group_manager/details/icomponent_controller.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent_event_publisher_consumer.hpp"
#include <stdint.h>

#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal
{

class MockProcessMonitor : IComponentController
{
  public:
    MOCK_METHOD(void, ctor, (IComponentEventPublisher & event_queue), ());
    MOCK_METHOD(void, dtor, (), ());
    MOCK_METHOD(void, doWork, (ComponentTask && task), (override));
    MOCK_METHOD(void, terminated, (IComponent & component, int32_t status), (override));
    // Capture the event queue so we can push to it during tests.
    IComponentEventPublisher* event_queue_;
};

extern std::unique_ptr<MockProcessMonitor> g_mock_process_monitor;

class ProcessMonitor final : public IComponentController
{
  public:
    explicit ProcessMonitor(IComponentEventPublisher& event_queue)
    {
        if (g_mock_process_monitor)
        {
            g_mock_process_monitor->event_queue_ = &event_queue;
            g_mock_process_monitor->ctor(event_queue);
        }
    }

    ~ProcessMonitor() override
    {
        g_mock_process_monitor->dtor();
    }

    void doWork(ComponentTask&& task) override
    {
        g_mock_process_monitor->doWork(std::move(task));
    }

    void terminated(IComponent& component, int32_t status) override
    {
        g_mock_process_monitor->terminated(component, status);
    }
};

}  // namespace score::mw::lifecycle::internal

#endif  // PROCESS_MONITOR_HPP_INCLUDED
