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
/// for inter-process communication. It uses semaphores to manage synchronization,
/// a process ID to identify the communicating process, and a flag to manage file descriptor closure.
struct IpcCommsSync final
{
    /// @brief Semaphore for synchronizing replies.
    /// The `reply_sync_` semaphore is used to synchronize the reception of replies
    /// from a communicating process. It ensures that the reply is received before
    /// proceeding with the next operation.
    Semaphore reply_sync_;

    /// @brief Semaphore for synchronizing sends.
    /// The `send_sync_` semaphore is used to synchronize the sending of messages
    /// to a communicating process. It ensures that the send operation is completed
    /// before proceeding with the next operation.
    Semaphore send_sync_;

    /// @brief Process ID of the communicating process.
    /// The `pid_` identifies the process involved in the communication. It is used
    /// to uniquely identify and manage the process during communication operations.
    ProcessID pid_;

    /// @brief Type of communications used for this process.
    /// The `comms_type_` member identifies whether the process has no communications
    /// with Launch Manager (`kNoComms`) or is expected to report kRunning (`kReporting`).
    CommsType comms_type_;

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
        IpcCommsP ret = nullptr;
        void* buf = mmap(nullptr, sizeof(IpcCommsSync), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        if (MAP_FAILED != buf)
        {
            ret = IpcCommsP(static_cast<IpcCommsSync*>(buf), IpcCommsDeletor());
        }

        return ret;
    }

    /// @brief Initializes semaphores within a given shared memory block.
    /// @param[in] block Pointer to the shared memory block where semaphores will be initialized.
    /// @return True if semaphore initialization is successful, false otherwise.
    /// @details This is static instead of a member function because, even though not needed currently, any vtable
    /// lookups would be UB.
    static bool initializeSemaphores(IpcCommsP shared_block)
    {
        bool result = true;

        if (osal::OsalReturnType::kFail == shared_block->send_sync_.init(0U, true) ||
            osal::OsalReturnType::kFail == shared_block->reply_sync_.init(0U, true))
        {
            result = false;
            LM_LOG_ERROR() << "Semaphore init failed: Unable to initialize send_sync or reply_sync semaphore.";
        }

        return result;
    }

    /// @brief Deinitializes semaphores within a given shared memory block.
    /// @param[in] block Pointer to the shared memory block.
    /// @details This is static instead of a member function because, even though not needed currently, any vtable
    /// lookups would be UB.
    static void deinit(IpcCommsP shared_block)
    {
        // We are not interested in the result of msync, just whether it worked or not.
        // If it did not work, the child process has probably crashed and corrupted the shared memory
        // so we should not try to deinitialize the semaphores.
        // mincore would be more appropriate here, but is not available on QNX
        if (msync(shared_block.get(), sizeof(IpcCommsSync), MS_ASYNC) == 0)
        {
            if (shared_block->send_sync_.deinit() != OsalReturnType::kSuccess)
            {
                LM_LOG_WARN() << "Failed to deinitialize send_sync semaphore.";
            }
            if (shared_block->reply_sync_.deinit() != OsalReturnType::kSuccess)
            {
                LM_LOG_WARN() << "Failed to deinitialize reply_sync semaphore.";
            }
        }
        else
        {
            LM_LOG_WARN() << "Skipping semaphore deinitialization - shared memory region appears invalid:"
                          << errno_message(errno);
        }
    }

  private:
    /// @brief Deleter to release IpcCommsSync object
    /// This is passed to the constructor of a shared pointer
    struct IpcCommsDeletor
    {
        void operator()(IpcCommsSync* ptr) const
        {
            if (nullptr != ptr)
            {
                if (munmap(ptr, sizeof(IpcCommsSync)) == -1)
                {
                    LM_LOG_ERROR() << "Unmapping of shared memory failed";
                }
            }
        }
    };
};

}  // namespace score::mw::lifecycle::internal::osal

#endif  // OSAL_IPC_COMMS_HPP_INCLUDED
