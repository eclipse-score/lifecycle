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

#ifndef MOCK_CHECKPOINT_IPC_SERVER_HPP
#define MOCK_CHECKPOINT_IPC_SERVER_HPP

#include <cstdint>
#include <string>

#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal::saf::ifappl
{

enum class EMockIpcInitResult : std::uint8_t
{
    kOk = 0U,
    kFailed = 1U,
    kPermissionDenied = 2U
};

enum class EMockIpcPeekResult : std::uint8_t
{
    kOk = 0U,
    kNoDataToRead = 1U,
    kIpcError = 2U
};

struct CheckpointBufferElement final
{
    /// @brief Timestamp of the checkpoint
    std::chrono::nanoseconds timestamp{0U};

    /// @brief Default constructor needed for storage in vector
    CheckpointBufferElement() = default;

    /// @brief Constructor for usage with emplace
    /// @param [in] f_timestamp The checkpoint timestamp
    CheckpointBufferElement(std::chrono::nanoseconds f_timestamp) noexcept(true) : timestamp(f_timestamp)
    {
    }
};

class MockCheckpointIpcServer
{
  public:
    using Payload = CheckpointBufferElement;
    using EIpcInitResult = EMockIpcInitResult;
    using EIpcPeekResult = EMockIpcPeekResult;
    MOCK_METHOD(std::string_view, getPath, (), (const, noexcept(true)));
    MOCK_METHOD(bool, hasOverflow, (), (noexcept(true)));
    MOCK_METHOD(EIpcPeekResult, peek, (Payload * &f_elem_p), (noexcept(true)));
    MOCK_METHOD(EIpcInitResult, init, (const std::string& f_ipcName_r, mode_t f_mode), (noexcept(false)));
    MOCK_METHOD(bool, setAccessRights, (uid_t f_uid), (noexcept(false)));
    MOCK_METHOD(bool, pop, (), (noexcept(true)));
};

extern std::unique_ptr<MockCheckpointIpcServer> g_mock_checkpoint_ipc_server;

class CheckpointIpcServer
{
  public:
    using Payload = CheckpointBufferElement;
    using EIpcInitResult = EMockIpcInitResult;
    using EIpcPeekResult = EMockIpcPeekResult;

    std::string_view getPath() const noexcept(true)
    {
        return g_mock_checkpoint_ipc_server->getPath();
    }

    bool hasOverflow() noexcept(true)
    {
        return g_mock_checkpoint_ipc_server->hasOverflow();
    }

    EIpcPeekResult peek(Payload*& f_elem_p) noexcept(true)
    {
        return g_mock_checkpoint_ipc_server->peek(f_elem_p);
    }

    EIpcInitResult init(const std::string& f_ipcName_r, mode_t f_mode) noexcept(false)
    {
        return g_mock_checkpoint_ipc_server->init(f_ipcName_r, f_mode);
    }

    bool setAccessRights(uid_t f_uid) noexcept(false)
    {
        return g_mock_checkpoint_ipc_server->setAccessRights(f_uid);
    }

    bool pop() noexcept(true)
    {
        return g_mock_checkpoint_ipc_server->pop();
    }
};

}  // namespace score::mw::lifecycle::internal::saf::ifappl

#endif
