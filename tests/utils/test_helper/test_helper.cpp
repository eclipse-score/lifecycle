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
#include "tests/utils/test_helper/test_helper.hpp"

#include <cerrno>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <thread>

namespace
{
bool event_received = false;
RunTargetActivationSource event_source;
RunTargetName event_target;
std::mutex event_mutex;
std::condition_variable event_condition;
}  // namespace

testing::AssertionResult touch_file(const std::string_view file_path)
{
    auto openRes = fopen(file_path.begin(), "w+");
    if (!openRes)
    {
        return testing::AssertionFailure()
               << "Could not touch file " << file_path << " errno: " << errno << " message: " << strerror(errno);
    }

    if (fclose(openRes) != 0)
    {
        return testing::AssertionFailure()
               << "Couldn't close opened file " << file_path << " errno: " << errno << " message: " << strerror(errno);
    }
    return testing::AssertionSuccess();
}

testing::AssertionResult check_clean(const std::initializer_list<std::string_view> files, const bool strict)
{
    std::stringstream failures{};
    for (const auto file : files)
    {
        if (!std::filesystem::exists(file))
        {
            continue;
        }

        if (strict)
        {
            failures << "'" << file << "' already exists!\n";
        }
        else if (!std::filesystem::remove(file))
        {
            failures << "Failed to remove '" << file << "'!\n";
        }
    }
    if (failures.tellp() > 0)
    {
        return testing::AssertionFailure() << failures.str();
    }
    return testing::AssertionSuccess();
}

testing::AssertionResult wait_for_files(
    const std::initializer_list<std::string_view> files,
    const std::chrono::milliseconds timeout)
{
    constexpr auto kPollInterval = std::chrono::milliseconds(10);
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true)
    {
        std::stringstream missing{};
        for (const auto file : files)
        {
            if (!std::filesystem::exists(file))
            {
                missing << "'" << file << "' ";
            }
        }

        if (missing.tellp() == 0)
        {
            return testing::AssertionSuccess();
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return testing::AssertionFailure() << "Not created within " << timeout.count() << " ms: " << missing.str();
        }
        std::this_thread::sleep_for(kPollInterval);
    }
}

testing::AssertionResult wait_for_file(const std::string_view file, const std::chrono::milliseconds timeout)
{
    return wait_for_files({file}, timeout);
}

void push_event(RunTargetActivationSource source, RunTargetName target)
{
    {
        std::cout << __FUNCTION__ << "Locking" << std::endl;
        std::unique_lock event_lock(event_mutex);
        event_condition.wait(event_lock, [&] {
            return !event_received;
        });
        std::cout << __FUNCTION__ << "Got event" << std::endl;
        event_received = true;
        event_source = source;
        event_target = target;
    }
    event_condition.notify_one();
}

void pop_event(std::function<void(RunTargetActivationSource source, RunTargetName target)> callback)
{
    RunTargetActivationSource source;
    RunTargetName target;
    {
        std::cout << __FUNCTION__ << "Locking" << std::endl;
        std::unique_lock event_lock(event_mutex);
        event_condition.wait(event_lock, [&] {
            return event_received;
        });
        std::cout << __FUNCTION__ << "Got event" << std::endl;
        source = event_source;
        target = event_target;
        event_received = false;
    }
    event_condition.notify_one();
    // Invoked without holding the mutex so the callback may block or call push_event/pop_event.
    callback(source, target);
}
