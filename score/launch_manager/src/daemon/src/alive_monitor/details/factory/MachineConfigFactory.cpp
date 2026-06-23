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
#include "score/mw/launch_manager/alive_monitor/details/factory/MachineConfigFactory.hpp"

#include <cassert>
#include <limits>
#include <string_view>
#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"


namespace score
{
namespace lcm
{
namespace saf
{
namespace factory
{

namespace
{
static constexpr char const* kLogPrefix{"Factory for FlatCfg MachineConfig:"};
}  // namespace

MachineConfigFactory::MachineConfigFactory() noexcept(true) : watchdog::IDeviceConfigFactory()
{
}

bool MachineConfigFactory::init(const score::mw::launch_manager::configuration::Config& config) noexcept(false)
{
    const auto& alive_sup = config.aliveSupervision();
    assert(alive_sup.evaluation_cycle_ms != 0U && "evaluation_cycle_ms must not be zero");
    cycleTimeNs = timers::TimeConversion::convertMilliSecToNanoSec(static_cast<double>(alive_sup.evaluation_cycle_ms));

    const auto& wd_opt = config.watchdog();
    if (wd_opt.has_value())
    {
        const auto& wd = *wd_opt;
        watchdog::DeviceConfig wdConfig{};
        wdConfig.fileName = wd.device_file_path;
        assert(wd.max_timeout_ms <= std::numeric_limits<std::uint16_t>::max());
        wdConfig.timeoutMax = static_cast<std::uint16_t>(wd.max_timeout_ms);
        wdConfig.canBeDeactivated = wd.deactivate_on_shutdown;
        wdConfig.needsMagicClose = wd.require_magic_close;
        watchdogConfigs.push_back(std::move(wdConfig));
    }

    logger_r.LogInfo() << kLogPrefix << "Loading of HM Machine Configuration succeeded.";
    logConfiguration();
    return true;
}

std::optional<watchdog::IDeviceConfigFactory::DeviceConfigurations>
MachineConfigFactory::getDeviceConfigurations() const
{
    return watchdogConfigs;
}

timers::NanoSecondType MachineConfigFactory::getCycleTimeInNs() const noexcept(true)
{
    return cycleTimeNs;
}

const MachineConfigFactory::SupervisionBufferConfig& MachineConfigFactory::getSupervisionBufferConfig() const
    noexcept(true)
{
    return supBufferCfg;
}

void MachineConfigFactory::logConfiguration() noexcept(true)
{
    logger_r.LogDebug() << kLogPrefix << "Alive Supervision buffer size:" << supBufferCfg.bufferSizeAliveSupervision;
    logger_r.LogDebug() << kLogPrefix << "Monitor buffer size:" << supBufferCfg.bufferSizeMonitor;
    logger_r.LogDebug() << kLogPrefix << "Periodicity:" << getCycleTimeInNs() << "ns";
    logger_r.LogDebug() << kLogPrefix << "Configured watchdogs:" << watchdogConfigs.size();
    std::uint32_t wdgCount{1U};
    for (const auto& wdgConfig : watchdogConfigs)
    {
        const std::string_view wdgMagicCloseBool{wdgConfig.needsMagicClose ? "true" : "false"};
        const std::string_view wdgDeactivatedBool{wdgConfig.canBeDeactivated ? "true" : "false"};
        logger_r.LogDebug() << kLogPrefix << "Watchdog" << wdgCount << "- device file:" << wdgConfig.fileName;
        logger_r.LogDebug() << kLogPrefix << "Watchdog" << wdgCount << "- max timeout:" << wdgConfig.timeoutMax << "ms";
        logger_r.LogDebug() << kLogPrefix << "Watchdog" << wdgCount << "- needs magic close:" << wdgMagicCloseBool;
        logger_r.LogDebug() << kLogPrefix << "Watchdog" << wdgCount
                            << "- deactivate on hm shutdown:" << wdgDeactivatedBool;
        ++wdgCount;
    }

    if (watchdogConfigs.empty())
    {
        logger_r.LogWarn() << kLogPrefix << "No watchdog configured";
    }
}

}  // namespace factory
}  // namespace saf
}  // namespace lcm
}  // namespace score
