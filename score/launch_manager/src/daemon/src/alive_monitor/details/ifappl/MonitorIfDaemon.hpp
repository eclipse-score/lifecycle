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

#ifndef MonitorIfDaemon_HPP_INCLUDED
#define MonitorIfDaemon_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/DataStructures.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"

namespace score::mw::lifecycle::internal::saf::ifappl
{

/// @brief Reads checkpoints from IPC channel and pushes them to attached observers
class MonitorIfDaemon : public common::Observer<ifexm::ObservableEvent>, public common::Observable<Checkpoint>
{
  public:
    /// @brief No Default Constructor
    MonitorIfDaemon() = delete;

    /// @brief No Copy Constructor
    MonitorIfDaemon(const MonitorIfDaemon&) = delete;
    /// @brief No Copy Assignment
    MonitorIfDaemon& operator=(const MonitorIfDaemon&) = delete;
    /// @brief No Move Assignment
    MonitorIfDaemon& operator=(MonitorIfDaemon&&) = delete;

    /// @brief Constructor
    /// @param [in] f_ipcServer_r       IPC interface
    /// @param [in] f_interfaceName_p   Interface name
    /// @throws std::bad_alloc in case of insufficient memory for string allocation
    /// @warning Please ensure that the pointer argument passed to the constructor is always a valid
    /// pointer, as the constructor doesn't check for null pointer access!
    explicit MonitorIfDaemon(CheckpointIpcServer& f_ipcServer_r, const char* f_interfaceName_p) noexcept(false);

    /// @brief Default Move Constructor
    MonitorIfDaemon(MonitorIfDaemon&&) = default;

    /// @brief Default Destructor
    ~MonitorIfDaemon() override = default;

    /// @brief Get interface Name
    /// @return     Interface name as string
    IdentifierHash getIdentifier() const noexcept(true) override;

    /// @brief Update data received from ObservableEvent
    /// @param [in]  f_observable_r ObservableEvent object which has send the update
    void updateData(const ifexm::ObservableEvent& f_observable_r) noexcept(true) override;

    /// @brief Check for new data
    /// @details Check Alive interface for new data from application side
    /// @param [in]  f_syncTimestamp    Timestamp till data shall be read, newer data will not be considered
    void checkForNewData(const std::chrono::nanoseconds f_syncTimestamp) noexcept(true);

  private:
    /// @brief Check if checkpoint ring buffer overflow has occurred
    /// @details The tailing read index (readIndex - 1) is always expected to be 0 (Clearing must be assured).
    /// This acts as a marker, if the marker is written with a timestamp the ring buffer was completely filled
    /// or in worst case completely overwritten.
    /// @return bool        true: Overflow occurred
    ///                     false: No overflow detected.
    bool isBufferOverflowed(void) const;

    /// @brief Push overflow event information to all checkpoint observer
    /// @details Every attached checkpoint observer will be informed that a data loss event in the
    /// Alive interface has occurred
    void pushOverflowInfoToObservers();

    /// @brief Move to kInactiveOverflow state and push overflow event to observers
    void handleOverflow(void);

    /// @brief Read checkpoints from the IPC buffer, pushing to observers, while there are entries within the timestamp
    /// to read.
    /// @param [in]  f_syncTimestamp        Timestamp till data shall be read, newer data will not be considered
    /// @returns True if successful, false if an error reading from the buffer occurred.
    bool readIpcUntil(const std::chrono::nanoseconds f_syncTimestamp);

    /// Internal states for instances of this class
    enum class EInternalState : std::uint8_t
    {
        kActive,           ///< Interface active
        kInactive,         ///< Interface inactive
        kInactiveOverflow  ///< Interface inactive due to overflow
    };

    /// Current internal state
    EInternalState status{EInternalState::kInactive};

    /// @brief Current Activation request
    bool isActivateRequest{false};

    /// @brief Current Deactivation request
    bool isDeactivateRequest{false};

    /// @brief Process restart status
    bool isProcessRestarted{false};

    /// Interface name
    const IdentifierHash k_interfaceName;

    /// @brief IPC connection to application
    CheckpointIpcServer& ipcserver_r;
};

}  // namespace score::mw::lifecycle::internal::saf::ifappl

#endif
