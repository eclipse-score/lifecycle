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

#ifndef OSAL_IPC_COMMS_HPP_INCLUDED
#define OSAL_IPC_COMMS_HPP_INCLUDED

#include <sys/mman.h>
#include <memory>

#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/osal/semaphore.hpp"

namespace score::mw::lifecycle::internal::osal
{

struct IpcCommsSync;
using IpcCommsP = std::shared_ptr<IpcCommsSync>;

/// @brief Structure for managing inter-process communication synchronization.
/// The `IpcCommsSync` structure is designed to handle synchronization mechanisms required
/// for inter-process communication. It uses a semaphore to signal kRunning and
/// a process ID to identify the communicating process.
/// It is only created for processes that are expected to report kRunning.
struct IpcCommsSync final
{
    /// @brief Semaphore used by the process to report kRunning.
    /// The communicating process posts this semaphore exactly once to signal that it has finished initialization.
    /// Launch Manager waits on it (and also posts it to wake the waiter up early if the process terminates).
    Semaphore send_sync_;

    /// @brief Process ID of the communicating process.
    /// The `pid_` is written by the child process right after fork. The client checks it against its own pid.
    /// The `pid_` is used to verify that the data is valid.
    ProcessID pid_;

    /// @brief Constant for the synchronization file descriptor.
    /// The `sync_fd` is a constant representing the file descriptor used for synchronization
    /// during communication. It is set to a value of 111 by default.
    static const int sync_fd = 111;

    // Cannot construct or destruct objects of this type

    /// @brief Constructor (deleted)
    /// These objects are only created in-place using a shared pointer constructor
    IpcCommsSync() = delete;

    /// @brief Copy constructor (deleted)
    IpcCommsSync(const IpcCommsSync&) = delete;

    /// @brief Move constructor (deleted)
    IpcCommsSync(IpcCommsSync&&) = delete;

    /// @brief Copy assignment operator (deleted)
    IpcCommsSync& operator=(const IpcCommsSync&) = delete;

    /// @brief Move assignment operator (deleted)
    IpcCommsSync& operator=(const IpcCommsSync&&) = delete;

    /// @brief Destructor (deleted)
    /// These objects are managed by a shared pointer and destroyed using a deleter
    ~IpcCommsSync() = delete;

    /// @brief Creation method to return a shared pointer to the comms object
    /// These objects are only ever created in-place in shared memory mapped from a
    /// file descriptor. The Lifecycle Client library will use the default file descriptor
    /// whereas Launch Manager supplies whatever file descriptor it is using for
    /// the process it is creating.
    static IpcCommsP getCommsObject(int fd = IpcCommsSync::sync_fd)
    {
        IpcCommsSync* const block = map(fd);
        return (nullptr != block) ? IpcCommsP(block, IpcCommsDeletor{}) : nullptr;
    }

    /// @brief Creates the comms object for Launch Manager, which owns the semaphore.
    /// @param[in] fd File descriptor of the shared memory object, already sized to sizeof(IpcCommsSync).
    /// @return Shared pointer to the initialized object, or nullptr if mapping or semaphore initialization failed.
    /// @details In contrast to getCommsObject(), the returned pointer destroys the semaphore and unmaps the memory
    /// when the last reference is dropped. Launch Manager only does this once the child process has terminated, so
    /// the process can no longer be inside a post on the semaphore.
    static IpcCommsP createCommsObject(int fd)
    {
        IpcCommsSync* const block = map(fd);
        if (nullptr == block)
        {
            return nullptr;
        }

        if (osal::OsalReturnType::kFail == block->send_sync_.init(0U, true))
        {
            LM_LOG_ERROR() << "Semaphore init failed: Unable to initialize send_sync semaphore.";
            IpcCommsDeletor{}(block);
            return nullptr;
        }

        return IpcCommsP(block, IpcCommsOwnerDeletor{});
    }

  private:
    /// @brief Maps the shared memory behind fd as an IpcCommsSync object.
    /// @return Pointer to the mapped object, or nullptr if mapping failed.
    static IpcCommsSync* map(int fd)
    {
        void* const buf = mmap(nullptr, sizeof(IpcCommsSync), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        return (MAP_FAILED != buf) ? static_cast<IpcCommsSync*>(buf) : nullptr;
    }

    /// @brief Deleter to release IpcCommsSync object
    /// This is used by the application process, which only maps the memory.
    /// Passed to the constructor of a shared pointer
    struct IpcCommsDeletor
    {
        void operator()(IpcCommsSync* ptr) const
        {
            if ((nullptr != ptr) && (munmap(ptr, sizeof(IpcCommsSync)) == -1))
            {
                LM_LOG_ERROR() << "Unmapping of shared memory failed";
            }
        }
    };

    /// @brief Deleter used by Launch Manager, which owns the semaphore.
    /// Destroys the semaphore first, then unmaps the memory.
    struct IpcCommsOwnerDeletor
    {
        void operator()(IpcCommsSync* ptr) const
        {
            if (nullptr == ptr)
            {
                return;
            }

            // We are not interested in the result of msync, just whether it worked or not.
            // If it did not work, the child process has probably crashed and corrupted the shared memory
            // so we should not try to deinitialize the semaphore.
            // mincore would be more appropriate here, but is not available on QNX
            if (msync(ptr, sizeof(IpcCommsSync), MS_ASYNC) == 0)
            {
                if (ptr->send_sync_.deinit() != OsalReturnType::kSuccess)
                {
                    LM_LOG_WARN() << "Failed to deinitialize send_sync semaphore.";
                }
            }
            else
            {
                LM_LOG_WARN() << "Skipping semaphore deinitialization - shared memory region appears invalid:"
                              << errno_message(errno);
            }

            IpcCommsDeletor{}(ptr);
        }
    };
};

}  // namespace score::mw::lifecycle::internal::osal

#endif  // OSAL_IPC_COMMS_HPP_INCLUDED
