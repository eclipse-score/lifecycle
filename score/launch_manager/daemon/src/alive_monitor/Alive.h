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
#include <type_traits>
#include <utility>

#include "score/mw/launch_manager/alive_monitor/MonitorImplWrapper.h"

namespace score::mw::lifecycle
{

/// @brief Alive Class
template <typename EnumT>
class Alive
{
public:
    /// @brief Creation of an Alive.
    /// @param [in] instance  Instance specifier of the Alive
    /// @throws std::runtime_error in case of an error loading the process-specific configuration
    /// @throws std::bad_alloc in case of insufficient memory
    /* RULECHECKER_comment(0, 11, check_unique_ptr_construction, "monitorImplWrapperPtr uses unique pointer\
       as type-casting to void pointer from make_unique is not possible", true_no_defect) */
    explicit Monitor(const std::string_view& instance) noexcept(false) :
        monitorImplWrapperPtr(std::make_unique<MonitorImplWrapper>(instance))
    {
    }

    /// @brief The copy constructor for Alive shall not be used.
    Alive(const Alive& se) = delete;

    /// @brief Move constructor for Alive
    /// @param [in,out] se  The Alive object to be moved
    Alive(Alive&& se) noexcept :
        monitorImplWrapperPtr(std::move(se.monitorImplWrapperPtr))
    {
    }

    /// @brief The copy assignment operator for Alive shall not be used.
    Alive& operator=(const Alive& se) = delete;

    /// @brief Move assignment operator for Alive
    /// @param [in,out] se  The Alive object to be moved
    /// @return The moved Alive object
    Alive& operator=(Alive&& se) noexcept
    {
        if (this != &se)
        {
            monitorImplWrapperPtr.reset(nullptr);
            monitorImplWrapperPtr = std::move(se.monitorImplWrapperPtr);
        }

        return *this;
    }

    /// @brief Destructor of an Alive
    virtual ~Monitor() noexcept
    {
    }

    /// @brief Reports an occurrence of a Checkpoint
    /// @param [in] checkpointId  Checkpoint identifier.
    /// @remark Thread safety:
    /// Report Checkpoint is NOT thread safe.
    /// In case a Monitor is shared between threads or in case two Monitor's are constructed
    /// with the same instance specifier in different threads a common lock before calling ReportCheckpoint is required.
    void ReportCheckpoint(EnumT checkpointId) const noexcept
    {
        if (monitorImplWrapperPtr.get() != nullptr)
        {
            monitorImplWrapperPtr->ReportCheckpoint(static_cast<score::mw::lifecycle::Checkpoint>(checkpointId));
        }
    }

private:
    /// @brief Unique pointer to the wrapper of implementation class of Monitor
    std::unique_ptr<MonitorImplWrapper> monitorImplWrapperPtr;

    /// @brief Assert if Monitor class is constructed with an enumeration type
    static_assert(std::is_enum<EnumT>::value,
                  "Monitor class must be constructed with template type "
                  "which is an enumeration class!");

    /// @brief Underlying data type of the used template argument
    using underlyingCheckpointIdType = typename std::underlying_type<EnumT>::type;

    /// @brief Assert if enumeration used during the construction of Monitor class
    ///        is of the type std::uint32_t
    static_assert(std::is_same<underlyingCheckpointIdType, score::mw::lifecycle::Checkpoint>::value,
                  "The enumeration class used during the construction of Monitor class must be of "
                  "type 'std::uint32_t'!");
};

#ifdef __cplusplus
extern "C"
{
#endif
    void* score_lcm_monitor_initialize(const char* instanceSpecifier) noexcept;
    void score_lcm_monitor_deinitialize(void* instance) noexcept;
    void score_lcm_monitor_report_checkpoint(void* instance, std::uint32_t checkpointId) noexcept;
#ifdef __cplusplus
}
#endif

}  // namespace score::mw::lifecycle
#endif  // SCORE_LCM_ALIVE_H_
