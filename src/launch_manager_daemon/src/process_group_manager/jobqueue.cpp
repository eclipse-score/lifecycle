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

#include <cstdio>
#include <score/lcm/internal/config.hpp>
#include <score/lcm/internal/osal/semaphore.hpp>
#include "jobqueue.hpp"

namespace score {

namespace lcm {

namespace internal {

template <class T>
JobQueue<T>::JobQueue(std::size_t capacity)
    : num_items_(), num_spaces_(), in_index_(0), out_index_(0), capacity_(capacity), the_items_(capacity) {
    num_items_.init(0U, false);
    num_spaces_.init(static_cast<uint32_t>(capacity & 0xFFFFFFFFUL), false);
}

template <class T>
JobQueue<T>::~JobQueue() {
    num_spaces_.deinit();
    num_items_.deinit();
}

template <class T>
std::shared_ptr<T> JobQueue<T>::getJobFromQueue() {
    std::shared_ptr<T> result;

    if (osal::OsalReturnType::kSuccess == num_items_.wait() && is_running_) {
        std::size_t index = static_cast<std::size_t>(out_index_.fetch_add(1U, std::memory_order_relaxed)) % capacity_;

        result = std::atomic_load_explicit(&the_items_[index], std::memory_order_acquire);
        num_spaces_.post();
    }

    return result;
}

template <class T>
bool JobQueue<T>::addJobToQueue(std::shared_ptr<T> job) {
    bool result = false;

    if (osal::OsalReturnType::kSuccess == num_spaces_.timedWait(score::lcm::internal::kMaxQueueDelay)) {
        std::size_t index = static_cast<std::size_t>(in_index_.fetch_add(1U, std::memory_order_relaxed)) % capacity_;

        std::atomic_store_explicit(&the_items_[index], job, std::memory_order_release);
        num_items_.post();
        result = true;
    }

    return result;
}

template <class T>
void JobQueue<T>::stopQueue(std::size_t nr_threads) {
    is_running_ = false;

    // Wake up all threads servicing this queue.
    // We do this by calling post as many times as we have threads.
    for (std::size_t i = 0; i < nr_threads; ++i) {
        num_items_.post();
    }
}

template <class T>
bool JobQueue<T>::isRunning() const {
    return is_running_;
}

class ProcessInfoNode;
template class JobQueue<ProcessInfoNode>;

}  // namespace lcm

}  // namespace internal

}  // namespace score
