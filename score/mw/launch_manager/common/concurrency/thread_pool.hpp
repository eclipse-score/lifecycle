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

#ifndef THREAD_POOL_HPP_INCLUDED
#define THREAD_POOL_HPP_INCLUDED

#include "score/mw/launch_manager/common/concurrency/mpmc_concurrent_queue.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/common/constants.hpp"
#include <memory>
#include <thread>
#include <vector>

namespace score::lcm::internal
{

/// @brief Templated thread pool for executing jobs from a queue.
/// This class manages a pool of threads that continuously retrieve and execute jobs
/// from an MPMCConcurrentQueue until the pool is stopped or destructed.
/// @tparam T The type of items stored in the queue (as std::shared_ptr<T>).
template <class T>
class ThreadPool final
{
    using Queue = MPMCConcurrentQueue<std::shared_ptr<T>, static_cast<std::size_t>(ProcessLimits::kMaxProcesses)>;

  public:
    /// @brief Constructs a ThreadPool with the specified number of threads.
    ///
    /// @param queue The queue from which threads will take work items.
    /// @param num_threads Number of threads in the pool.
    ThreadPool(std::shared_ptr<Queue> queue, uint32_t num_threads) : the_job_queue_(queue)
    {
        worker_threads_.reserve(num_threads);
        for (uint32_t i = 0U; i < num_threads; ++i)
        {
            static_cast<void>(i);
            worker_threads_.emplace_back(std::make_unique<std::thread>(&ThreadPool::run, this));
        }
    }

    /// @brief Destructor. Requests stop and joins all threads.
    ~ThreadPool()
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

    // Rule of five
    /// @brief Copy constructor is deleted.
    ThreadPool(const ThreadPool&) = delete;

    /// @brief Copy assignment operator is deleted.
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// @brief Move constructor is deleted.
    ThreadPool(ThreadPool&&) = delete;

    /// @brief Move assignment operator is deleted.
    ThreadPool& operator=(ThreadPool&&) = delete;

    /// @brief Requests all threads to stop.
    /// Calls stop() on the queue, which unblocks all threads waiting in pop().
    void stop()
    {
        static_cast<void>(the_job_queue_->stop());
    }

  private:
    /// @brief Entry point for each thread.
    /// Blocks on pop() until a job arrives or the queue is stopped, then executes the job.
    void run()
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

    /// @brief The queue from which each thread takes work.
    std::shared_ptr<Queue> the_job_queue_{};

    /// @brief Pool of worker threads.
    std::vector<std::unique_ptr<std::thread>> worker_threads_{};
};

}  // namespace score::lcm::internal

#endif  // THREAD_POOL_HPP_INCLUDED
