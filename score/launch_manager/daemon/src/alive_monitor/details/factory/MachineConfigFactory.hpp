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


#ifndef MACHINE_CONFIG_FACTORY_HPP_INCLUDED
#define MACHINE_CONFIG_FACTORY_HPP_INCLUDED

#include <optional>
#include "score/mw/launch_manager/alive_monitor/details/factory/StaticConfig.hpp"
#include "score/mw/launch_manager/alive_monitor/details/logging/PhmLogger.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include "score/mw/launch_manager/alive_monitor/details/watchdog/IDeviceConfigFactory.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"

namespace score
{
namespace lcm
{
namespace saf
{

namespace logging
{
class PhmLogger;
}

namespace factory
{

/// @brief Factory for loading the HM Machine Configuration from the unified launch_manager_config.bin
class MachineConfigFactory : public watchdog::IDeviceConfigFactory
{
public:
    /// @brief Holds different buffer sizes that may be configured in the HM Machine Config
    struct SupervisionBufferConfig
    {
        /// @brief Configured buffer size for alive supervisions
        std::uint16_t bufferSizeAliveSupervision{StaticConfig::k_DefaultAliveSupCheckpointBufferElements};
        /// @brief Configured buffer size for Monitor entities
        std::uint16_t bufferSizeMonitor{StaticConfig::k_DefaultMonitorBufferElements};
    };

    /// @brief Constructor
    MachineConfigFactory() noexcept(true);

    /// @brief Destructor
    ~MachineConfigFactory() override = default;

    /// @brief No Copy Constructor
    MachineConfigFactory(const MachineConfigFactory&) = delete;
    /// @brief No Copy Assignment
    MachineConfigFactory& operator=(const MachineConfigFactory&) = delete;
    /// @brief No Move Constructor
    MachineConfigFactory(MachineConfigFactory&&) = delete;
    /// @brief No Move Assignment
    MachineConfigFactory& operator=(MachineConfigFactory&&) = delete;

    /// @brief Load machine configuration from the unified Config object
    bool init(const score::mw::launch_manager::configuration::Config& config) noexcept(false);

    /// @copydoc IDeviceConfigFactory::getDeviceConfigurations()
    std::optional<watchdog::IDeviceConfigFactory::DeviceConfigurations> getDeviceConfigurations() const override;

    /// @brief Returns the configured hm daemon cycle time in nanoseconds
    timers::NanoSecondType getCycleTimeInNs() const noexcept(true);

    /// @brief Returns the configured buffer sizes for supervisions
    const SupervisionBufferConfig& getSupervisionBufferConfig() const noexcept(true);

private:
    /// @brief Log all configuration settings
    void logConfiguration() noexcept(true);

    /// @brief Configured watchdog devices
    watchdog::IDeviceConfigFactory::DeviceConfigurations watchdogConfigs{};

    /// @brief Configured HM Daemon cycle time
    timers::NanoSecondType cycleTimeNs{StaticConfig::k_hmDaemonDefaultCycleTime};

    /// @brief Configured supervision buffer sizes
    SupervisionBufferConfig supBufferCfg{};

    /// Logger object for logging messages
    logging::PhmLogger& logger_r{logging::PhmLogger::getLogger(logging::PhmLogger::EContext::factory)};
};

}  // namespace factory
}  // namespace saf
}  // namespace lcm
}  // namespace score

#endif
