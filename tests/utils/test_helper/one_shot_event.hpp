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

#ifndef TESTS_UTILS_TEST_HELPER_ONE_SHOT_EVENT_HPP
#define TESTS_UTILS_TEST_HELPER_ONE_SHOT_EVENT_HPP

#include <condition_variable>
#include <mutex>

/// @brief One-shot event used to synchronise threads without sleeping
class OneShotEvent
{
  public:
    /// @brief Unblock the waiting thread
    void Signal()
    {
        {
            std::lock_guard<std::mutex> lock{mutex_};
            set_ = true;
        }
        cv_.notify_all();
    }

    /// @brief Wait for the event to occur
    void Wait()
    {
        std::unique_lock<std::mutex> lock{mutex_};
        cv_.wait(lock, [this] {
            return set_;
        });
    }

  private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool set_{false};
};

#endif  // TESTS_UTILS_TEST_HELPER_ONE_SHOT_EVENT_HPP
