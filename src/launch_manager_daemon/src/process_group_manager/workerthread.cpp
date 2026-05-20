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

#include "workerthread.hpp"
#include "processinfonode.hpp"

namespace score
{

namespace lcm
{

namespace internal
{

template <class T>
WorkerThread<T>::WorkerThread(std::shared_ptr<Queue> queue, uint32_t num_threads) : the_job_queue_(queue)
{
    worker_threads_.reserve(num_threads);

    for (uint32_t i = 0U; i < num_threads; ++i)
    {
        static_cast<void>(i);
        worker_threads_.emplace_back(std::make_unique<std::thread>(&WorkerThread::run, this));
    }
}

template <class T>
WorkerThread<T>::~WorkerThread()
{
    stop();

    for (auto& thread : worker_threads_)
    {
        if (thread->joinable())
        {
            thread->join();
        }
    }
}

template <class T>
void WorkerThread<T>::stop()
{
    static_cast<void>(the_job_queue_->stop());
}

template <class T>
void WorkerThread<T>::run()
{
    while (true)
    {
        auto job = the_job_queue_->pop();
        if (!job)
        {
            if (job.error() == ConcurrencyErrc::kStopped)
            {
                break;
            }
            LM_LOG_ERROR() << "Got an error getting a job: " << job.error();
            continue;
        }
        (*job)->doWork();
    }
}

// Explicit instantiation for ProcessInfoNode
template class WorkerThread<ProcessInfoNode>;

}  // namespace internal

}  // namespace lcm

}  // namespace score
