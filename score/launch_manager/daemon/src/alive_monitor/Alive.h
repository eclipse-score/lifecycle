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

#ifndef SCORE_LCM_ALIVE_H_
#define SCORE_LCM_ALIVE_H_

#include <cstdint>
#include <memory>
#include <string_view>

namespace score::mw::lifecycle
{

// Forward declaration
class MonitorImpl;

/// @brief Alive Class
class Alive
{
public:
    /// @brief Creation of an Alive.
    /// @param [in] instance  Instance specifier of the Monitor
    /// @throws std::runtime_error in case of an error loading the process-specific configuration
    /// @throws std::bad_alloc in case of insufficient memory
    explicit Alive(const std::string_view& instance) noexcept(false);

    /// @brief The copy constructor for Alive shall not be used.
    Alive(const Alive& se) = delete;

    /// @brief Move constructor for Alive
    /// @param [in,out] se  The Alive object to be moved
    Alive(Alive&& se) noexcept;

    /// @brief The copy assignment operator for Alive shall not be used.
    Alive& operator=(const Alive& se) = delete;

    /// @brief Move assignment operator for Alive
    /// @param [in,out] se  The Alive object to be moved
    /// @return The moved Alive object
    Alive& operator=(Alive&& se) noexcept;

    /// @brief Destructor of an Alive
    virtual ~Alive() noexcept;

    /// @brief Reports an occurrence of a Checkpoint
    /// @param [in] checkpointId  Checkpoint identifier.
    /// @remark Thread safety:
    /// Report Checkpoint is NOT thread safe.
    /// In case a Monitor is shared between threads or in case two Monitor's are constructed
    /// with the same instance specifier in different threads a common lock before calling ReportCheckpoint is required.
    void ReportCheckpoint(std::uint32_t checkpointId) const noexcept;

private:
    /// @brief Unique pointer to implementation class of Monitor
    std::unique_ptr<MonitorImpl> monitorImplPtr;
};

#ifdef __cplusplus
extern "C"
{
#endif
    void* score_lcm_alive_initialize(const char* instanceSpecifier) noexcept;
    void score_lcm_alive_deinitialize(void* instance) noexcept;
    void score_lcm_alive_report_checkpoint(void* instance, std::uint32_t checkpointId) noexcept;
#ifdef __cplusplus
}
#endif

}  // namespace score::mw::lifecycle
#endif  // SCORE_LCM_ALIVE_H_
