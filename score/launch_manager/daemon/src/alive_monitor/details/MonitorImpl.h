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

#ifndef SCORE_LCM_MonitorImpl_H_
#define SCORE_LCM_MonitorImpl_H_

#include <memory>
#include <utility>

#include <string>
#include "score/mw/launch_manager/alive_monitor/details/ifappl/DataStructures.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ipc/IpcClient.hpp"
#include "score/mw/launch_manager/alive_monitor/details/logging/PhmLogger.hpp"

namespace score::mw::lifecycle
{

/// @brief Implementation class for score::mw::lifecycle::Monitor class
///        This class is responsible for establishing the connection between the application and PHM daemon
///        by invoking the calls to PHM class methods and to forward the reported checkpoints from the application
///        to PHM daemon for supervision evaluation
class MonitorImpl
{
  public:
    /// @brief The element that is sent via IPC
    using CheckpointBufferElement = score::lcm::saf::ifappl::CheckpointBufferElement;
    /// @brief The IPC Connection type
    using CheckpointIpcClient = score::lcm::saf::ipc::IpcClient<CheckpointBufferElement,
                                                                score::lcm::saf::ifappl::k_maxCheckpointBufferElements>;

    /// @brief Non-parametric constructor is not supported
    MonitorImpl() = delete;

    /// @brief Constructor of MonitorImpl class
    /// @param [in] f_instanceSpecifier_r  Instance specifier object with the metamodel path of
    ///                                    the Monitor
    /// @param [in] f_ipcClient            Ipc Connection to PHM daemon
    /// @throws std::runtime_error in case ipc path could not be read from configuration
    /// @throws std::bad_alloc in case of insufficient memory
    explicit MonitorImpl(
        const std::string_view& f_instanceSpecifier_r,
        std::unique_ptr<CheckpointIpcClient> f_ipcClient = std::make_unique<CheckpointIpcClient>()) noexcept(false);

    /// @brief The copy constructor for MonitorImpl is not supported.
    MonitorImpl(const MonitorImpl&) = delete;

    /// @brief The move constructor for MonitorImpl is not supported.
    MonitorImpl(MonitorImpl&&) = delete;

    /// @brief The copy assignment operator for MonitorImpl is not supported.
    MonitorImpl& operator=(const MonitorImpl&) & = delete;

    /// @brief The move assignment operator for MonitorImpl is not supported.
    MonitorImpl& operator=(MonitorImpl&&) & noexcept = delete;

    /// @brief Destructor of the class
    virtual ~MonitorImpl() = default;

    /// @brief Reports an occurrence of a Checkpoint
    /// @param [in] f_checkpointId   Checkpoint identifier.
    void ReportCheckpoint(std::uint32_t f_checkpointId) const noexcept(true);

  private:
    /// @brief Connect the application process with PHM daemon using IPC
    /// @throws std::runtime_error in case ipc path could not be read from configuration
    void connectToPhmDaemon(void) noexcept(false);

    /// @brief Read the Monitor Interface Path from an environment variable. 
    /// This is then used to initialise the IPC client
    /// @return Value of environment variable or nullopt if getenv fails
    static std::optional<std::string_view> readInterfacePath() noexcept;

    /// @brief Instance specifier path of the Monitor instance
    const std::string k_instanceSpecifierPath;

    /// @brief IPC Connection to PHM Daemon
    /// Class needs to be mutable to use in "const" reportCheckpoint method
    mutable std::unique_ptr<CheckpointIpcClient> ipcClient;
    /// Logger object
    score::lcm::saf::logging::PhmLogger& logger_r;
};

}  // namespace score::mw::lifecycle

#endif
