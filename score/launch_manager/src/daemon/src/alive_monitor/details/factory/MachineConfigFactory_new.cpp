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
#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/factory/MachineConfigFactory.hpp"

#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"
#include <cassert>
#include <string_view>

namespace score
{
namespace lcm
{
namespace saf
{
namespace factory
{

using Config = score::mw::launch_manager::configuration::Config;
using DeviceConfigurations = watchdog::IDeviceConfigFactory::DeviceConfigurations;
using NanoSecondType = timers::NanoSecondType;

namespace
{
static constexpr const std::string_view kLogPrefix{"Factory for FlatCfg MachineConfig:"};
}  // namespace

MachineConfigFactory::MachineConfigFactory() noexcept(true) : watchdog::IDeviceConfigFactory()
{
}

bool MachineConfigFactory::init(const Config& config) noexcept(false)
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
        wdConfig.timeoutMax = static_cast<std::uint16_t>(wd.max_timeout_ms);
        wdConfig.canBeDeactivated = wd.deactivate_on_shutdown;
        wdConfig.needsMagicClose = wd.require_magic_close;
        watchdogConfigs.push_back(std::move(wdConfig));
    }

    LM_LOG_INFO() << kLogPrefix << "Loading of HM Machine Configuration succeeded.";
    logConfiguration();
    return true;
}

std::optional<DeviceConfigurations> MachineConfigFactory::getDeviceConfigurations() const
{
    return watchdogConfigs;
}

NanoSecondType MachineConfigFactory::getCycleTimeInNs() const noexcept(true)
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
    LM_LOG_DEBUG() << kLogPrefix << "Alive Supervision buffer size:" << supBufferCfg.bufferSizeAliveSupervision;
    LM_LOG_DEBUG() << kLogPrefix << "Monitor buffer size:" << supBufferCfg.bufferSizeMonitor;
    LM_LOG_DEBUG() << kLogPrefix << "Periodicity:" << getCycleTimeInNs() << "ns";
    LM_LOG_DEBUG() << kLogPrefix << "Configured watchdogs:" << watchdogConfigs.size();
    std::uint32_t wdgCount{1U};
    for (const auto& wdgConfig : watchdogConfigs)
    {
        const std::string_view wdgMagicCloseBool{wdgConfig.needsMagicClose ? "true" : "false"};
        const std::string_view wdgDeactivatedBool{wdgConfig.canBeDeactivated ? "true" : "false"};
        LM_LOG_DEBUG() << kLogPrefix << "Watchdog" << wdgCount << "- device file:" << wdgConfig.fileName;
        LM_LOG_DEBUG() << kLogPrefix << "Watchdog" << wdgCount << "- max timeout:" << wdgConfig.timeoutMax << "ms";
        LM_LOG_DEBUG() << kLogPrefix << "Watchdog" << wdgCount << "- needs magic close:" << wdgMagicCloseBool;
        LM_LOG_DEBUG() << kLogPrefix << "Watchdog" << wdgCount << "- deactivate on hm shutdown:" << wdgDeactivatedBool;
        ++wdgCount;
    }

    if (watchdogConfigs.empty())
    {
        LM_LOG_WARN() << kLogPrefix << "No watchdog configured";
    }
}

}  // namespace factory
}  // namespace saf
}  // namespace lcm
}  // namespace score
