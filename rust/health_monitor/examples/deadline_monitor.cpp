// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <ffi/deadline_monitor.hpp>

int main()
{
    using namespace hm;
    using namespace std::chrono_literals;

    DeadlineMonitorBuilder builder{};
    DeadlineMonitor monitor = builder
        .add_deadline(Tag("deadline1"), DurationRange{ .min = 10ms, .max = 1000ms })
        .add_deadline(Tag("deadline2"), DurationRange{ .min = 50ms, .max = 250ms })
        .build();

    assert(monitor.status() == hm_Status::Running);
    assert(monitor.disable() == Error::NoError);
    assert(monitor.status() == hm_Status::Disabled);

    // Disable while disabled. Does nothing.
    assert(monitor.disable() == Error::NotAllowed);
    assert(monitor.status() == hm_Status::Disabled);

    assert(monitor.enable() == Error::NoError);
    assert(monitor.status() == hm_Status::Running);

    // Enable while enabled. Does nothing.
    assert(monitor.enable() == Error::NotAllowed);
    assert(monitor.status() == hm_Status::Running);

    std::thread t1(
        [&]()
        {
            auto d1 = monitor.get_deadline(Tag("deadline1"));
            assert(d1.has_value());
            auto d2 = monitor.get_deadline(Tag("deadline2"));
            assert(d2.has_value());

            // Run task 1.
            d1->start();
            std::this_thread::sleep_for(250ms);
            d1->stop();
            assert(monitor.status() == hm_Status::Running);

            // Run task 2.
            d2->start();
            std::this_thread::sleep_for(100ms);
            d2->stop();
            assert(monitor.status() == hm_Status::Running);

            // Run task 1 again.
            d1->start();
            std::this_thread::sleep_for(250ms);
            d1->stop();
            assert(monitor.status() == hm_Status::Running);
        });

    t1.join();

    std::thread t2(
        [&]()
        {
            auto d = monitor.get_deadline(Tag("deadline2"));
            assert(d.has_value());

            // This task is too long.
            d->start();
            std::this_thread::sleep_for(500ms);
            d->stop();
            assert(monitor.status() == hm_Status::Failed);
        });

    t2.join();

    assert(monitor.status() == hm_Status::Failed);

    return EXIT_SUCCESS;
}
