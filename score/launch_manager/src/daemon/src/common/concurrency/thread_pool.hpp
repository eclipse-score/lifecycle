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
#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent_controller.hpp"
#include <pthread.h>
#if defined(__QNX__)
#include <sys/storage.h>
#endif
#include <score/jthread.hpp>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

namespace score::mw::lifecycle::internal
{

/// @brief Templated worker thread pool for executing jobs from a queue.
/// This class manages a pool of worker threads that continuously retrieve and execute jobs
/// from an MPMCConcurrentQueue until the pool is stopped or destructed.
/// @tparam T The type of items stored in the queue (as std::shared_ptr<T>).
template <class T>
class ThreadPool final
{
    using Queue = MPMCConcurrentQueue<std::optional<T>, static_cast<std::size_t>(ProcessLimits::kMaxProcesses)>;

  public:
    /// @brief Constructs a ThreadPool with the specified number of threads.
    ///
    /// @param queue The MpmcQueue from which threads will take work items.
    /// @param num_threads Number of threads in the pool.
    /// @param component_controller_ The controller to delegate work to.
    ThreadPool(std::shared_ptr<Queue> queue, uint32_t num_threads, IComponentController& component_controller)
        : the_job_queue_(queue), component_controller_(component_controller)
    {
        worker_threads_.reserve(num_threads);
        for (uint32_t i = 0U; i < num_threads; ++i)
        {
            static_cast<void>(i);
            static_cast<void>(worker_threads_.emplace_back(
                score::cpp::jthread::stack_size_hint{kWorkerThreadStackSize},
                score::cpp::jthread::name_hint{"worker"},
                &ThreadPool::run,
                this));
        }
    }

    /// @brief Destructor.
    /// Requests stop and joins all worker threads.
    ~ThreadPool()
    {
        stop();
        for (auto& thread : worker_threads_)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
        // STACK-PROBE (temporary diagnostics)
        std::fprintf(
            stderr,
            "[stack-probe] pool max: used=%zu actual_size=%zu requested_size=%zu bytes\n",
            maxStackUsed().load(),
            maxStackSize().load(),
            static_cast<std::size_t>(kWorkerThreadStackSize));
    }

    // Rule of five
    /// @brief Copy constructor is deleted to prevent copying.
    ThreadPool(const ThreadPool&) = delete;

    /// @brief Copy assignment operator is deleted to prevent copying.
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// @brief Move constructor is deleted to prevent moving.
    ThreadPool(ThreadPool&&) = delete;

    /// @brief Move assignment operator is deleted to prevent moving.
    ThreadPool& operator=(ThreadPool&&) = delete;

    /// @brief Requests all worker threads to stop.
    /// Calls stop() on the queue, which unblocks all threads waiting in pop().
    void stop()
    {
        static_cast<void>(the_job_queue_->stop());
    }

  private:
    /// @brief Entry point for each worker thread.
    /// Blocks on pop() until a job arrives or the queue is stopped, then executes the job.
    // STACK-PROBE (temporary diagnostics)
    static std::atomic<std::size_t>& maxStackSize()
    {
        static std::atomic<std::size_t> max_size{0U};
        return max_size;
    }

    static std::atomic<std::size_t>& maxStackUsed()
    {
        static std::atomic<std::size_t> max_used{0U};
        return max_used;
    }

    // STACK-PROBE (temporary diagnostics): fills the not yet used part of the stack with a pattern.
    static __attribute__((noinline)) void paintStack(volatile std::uint8_t* low, volatile std::uint8_t* limit)
    {
        for (volatile std::uint8_t* p = low; p < limit; ++p)
        {
            *p = 0xA5U;
        }
    }

    void run()
    {
        // STACK-PROBE (temporary diagnostics)
        auto* const frame = static_cast<volatile std::uint8_t*>(__builtin_frame_address(0));
#if defined(__QNX__)
        // QNX has no pthread_getattr_np(); the thread local storage holds the stack bounds.
        void* const stack_addr = __tls()->__stackaddr;
        const std::size_t stack_size = __tls()->__stacksize;
#else
        pthread_attr_t attr{};
        void* stack_addr = nullptr;
        std::size_t stack_size = 0U;
        static_cast<void>(pthread_getattr_np(pthread_self(), &attr));
        static_cast<void>(pthread_attr_getstack(&attr, &stack_addr, &stack_size));
        static_cast<void>(pthread_attr_destroy(&attr));
#endif
        auto* const stack_low = static_cast<volatile std::uint8_t*>(stack_addr);
        auto* const stack_high = stack_low + stack_size;
        // Leave a margin below the current frame for paintStack() itself.
        if (stack_low < frame - 1024)
        {
            paintStack(stack_low, frame - 1024);
        }
        runLoop();
        volatile std::uint8_t* p = stack_low;
        while ((p < stack_high) && (*p == 0xA5U))
        {
            ++p;
        }
        const std::size_t used = static_cast<std::size_t>(stack_high - p);
        std::fprintf(
            stderr,
            "[stack-probe] worker: used=%zu actual_size=%zu requested_size=%zu bytes\n",
            used,
            stack_size,
            static_cast<std::size_t>(kWorkerThreadStackSize));
        std::size_t prev_size = maxStackSize().load();
        while ((prev_size < stack_size) && !maxStackSize().compare_exchange_weak(prev_size, stack_size))
        {
        }
        std::size_t prev = maxStackUsed().load();
        while ((prev < used) && !maxStackUsed().compare_exchange_weak(prev, used))
        {
        }
    }

    void runLoop()
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
                LM_LOG_ERROR() << "Got an error getting a job:" << job.error();
                continue;
            }
            component_controller_.doWork(std::move(**job));
        }
    }

    /// @brief The queue from which each thread takes work.
    std::shared_ptr<Queue> the_job_queue_{};

    IComponentController& component_controller_;

    /// @brief Vector of worker threads.
    std::vector<score::cpp::jthread> worker_threads_{};
};

}  // namespace score::mw::lifecycle::internal

#endif  // THREAD_POOL_HPP_INCLUDED
