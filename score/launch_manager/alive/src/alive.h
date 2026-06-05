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
class AliveImpl;

/// @brief Alive Class
class Alive
{
public:
    /// @brief Creation of an Alive.
    /// @param [in] instance  Instance specifier (currently unused)
    /// @throws std::runtime_error if the configured IPC channel to connect to launch manager is not existing
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

    /// @brief Reports an alive notification
    /// @remark Thread safety:
    /// This method is NOT thread safe.
    void ReportAlive() const noexcept;

private:
    /// @brief Unique pointer to implementation class of Alive
    std::unique_ptr<AliveImpl> aliveImplPtr;
};

#ifdef __cplusplus
extern "C"
{
#endif
    void* score_lcm_alive_initialize(const char* instanceSpecifier) noexcept;
    void score_lcm_alive_deinitialize(void* instance) noexcept;
    void score_lcm_alive_report_alive(void* instance) noexcept;
#ifdef __cplusplus
}
#endif

}  // namespace score::mw::lifecycle
#endif  // SCORE_LCM_ALIVE_H_
