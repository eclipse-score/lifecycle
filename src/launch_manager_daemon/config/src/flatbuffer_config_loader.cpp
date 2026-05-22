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
#include "flatbuffer_config_loader.hpp"
#include "new_lm_flatcfg_generated.h"

#include "score/os/errno.h"

#include <score/lcm/internal/log.hpp>

#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <sys/types.h>
#include <vector>

namespace score::launch_manager::config
{

namespace fb = score::launch_manager::config::fb;

namespace
{

constexpr int32_t kExpectedSchemaVersion = 1;
constexpr double kSecondsToMilliseconds = 1000.0;

uint32_t secondsToMs(double seconds)
{
    const auto result = static_cast<uint32_t>(seconds * kSecondsToMilliseconds);
    assert(!(seconds > 0.0 && result == 0U) && "Sub-millisecond precision is not supported: value rounds to 0ms");
    return result;
}

// --- Enum conversion helpers ---

ApplicationType convertApplicationType(fb::ApplicationType fb_type)
{
    switch (fb_type)
    {
        case fb::ApplicationType::Reporting:
            return ApplicationType::Reporting;
        case fb::ApplicationType::Reporting_And_Supervised:
            return ApplicationType::ReportingAndSupervised;
        case fb::ApplicationType::State_Manager:
            return ApplicationType::StateManager;
        case fb::ApplicationType::Native:
        default:
            return ApplicationType::Native;
    }
}

ProcessState convertProcessState(fb::ProcessState fb_state)
{
    switch (fb_state)
    {
        case fb::ProcessState::Terminated:
            return ProcessState::Terminated;
        case fb::ProcessState::Running:
        default:
            return ProcessState::Running;
    }
}

// --- String/vector helpers ---

std::string safeString(const ::flatbuffers::String* s)
{
    return s ? s->str() : std::string{};
}

std::vector<std::string> convertStringVector(
    const ::flatbuffers::Vector<::flatbuffers::Offset<::flatbuffers::String>>* vec)
{
    std::vector<std::string> result;
    if (vec != nullptr)
    {
        result.reserve(vec->size());
        for (const auto* s : *vec)
        {
            result.emplace_back(s ? s->str() : std::string{});
        }
    }
    return result;
}

score::cpp::expected<std::vector<gid_t>, IConfigLoader::Error> convertGidVector(
    const ::flatbuffers::Vector<uint32_t>* vec)
{
    std::vector<gid_t> result;
    if (vec != nullptr)
    {
        result.reserve(vec->size());
        for (const auto val : *vec)
        {
            if (val < std::numeric_limits<gid_t>::min() || val > std::numeric_limits<gid_t>::max())
            {
                LM_LOG_ERROR() << "Sandbox supplementary group id " << val << " is out of valid gid_t range [" << std::numeric_limits<gid_t>::min() << "," << std::numeric_limits<gid_t>::max() << "]";
                return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
            }
            result.emplace_back(static_cast<gid_t>(val));
        }
    }
    return result;
}

Environment convertEnvironmentalVariables(
    const ::flatbuffers::Vector<::flatbuffers::Offset<fb::EnvironmentalVariable>>* vec)
{
    Environment result;
    if (vec != nullptr)
    {
        result.reserve(vec->size());
        for (const auto* ev : *vec)
        {
            if (ev != nullptr)
            {
                assert(ev->key() && "EnvironmentalVariable::key must never be nullptr as it is required in the schema");
                assert(ev->value() && "EnvironmentalVariable::value must never be nullptr as it is required in the schema");
                result.add(ev->key()->str(), ev->value()->str());
            }
        }
    }
    return result;
}

// --- Recovery action conversion ---

std::optional<RestartAction> convertRestartAction(const fb::RestartAction* ra)
{
    if (ra == nullptr)
    {
        return std::nullopt;
    }
    return RestartAction{ra->number_of_attempts(), secondsToMs(ra->delay_before_restart())};
}

std::optional<SwitchRunTargetAction> convertSwitchRunTargetAction(const fb::SwitchRunTargetAction* sa)
{
    if (sa == nullptr)
    {
        return std::nullopt;
    }
    assert(sa->run_target() && "SwitchRunTargetAction::run_target must never be nullptr as it is required in the schema");
    return SwitchRunTargetAction{sa->run_target()->str()};
}

SwitchRunTargetAction convertRequiredSwitchRunTargetAction(const fb::SwitchRunTargetAction* sa)
{
    assert(sa && "SwitchRunTargetAction must never be nullptr as it is required in the schema");
    return convertSwitchRunTargetAction(sa).value();
}

// --- Struct conversion helpers ---

ComponentAliveSupervision convertComponentAliveSupervision(const fb::ComponentAliveSupervision* fb_cas)
{
    ComponentAliveSupervision result{};
    if (fb_cas != nullptr)
    {
        result.reporting_cycle_ms = secondsToMs(fb_cas->reporting_cycle());
        result.failed_cycles_tolerance = fb_cas->failed_cycles_tolerance();
        result.min_indications = fb_cas->min_indications().value_or(0U);
        result.max_indications = fb_cas->max_indications().value_or(0U);
    }
    return result;
}

ApplicationProfile convertApplicationProfile(const fb::ApplicationProfile* fb_ap)
{
    ApplicationProfile result{};
    if (fb_ap != nullptr)
    {
        result.application_type = convertApplicationType(fb_ap->application_type());
        result.is_self_terminating = fb_ap->is_self_terminating();
        if (fb_ap->alive_supervision() != nullptr)
        {
            result.alive_supervision = convertComponentAliveSupervision(fb_ap->alive_supervision());
        }
    }
    return result;
}

ReadyCondition convertReadyCondition(const fb::ReadyCondition* fb_rc)
{
    ReadyCondition result{ProcessState::Running};
    if (fb_rc != nullptr)
    {
        const auto opt = fb_rc->process_state();
        if (opt.has_value())
        {
            result.process_state = convertProcessState(*opt);
        }
    }
    return result;
}

ComponentProperties convertComponentProperties(const fb::ComponentProperties* fb_cp)
{
    ComponentProperties result{};
    if (fb_cp != nullptr)
    {
        assert(fb_cp->binary_name() && "ComponentProperties::binary_name must never be nullptr as it is required in the schema");
        assert(fb_cp->application_profile() && "ComponentProperties::application_profile must never be nullptr as it is required in the schema");
        assert(fb_cp->ready_condition() && "ComponentProperties::ready_condition must never be nullptr as it is required in the schema");
        result.binary_name = fb_cp->binary_name()->str();
        result.application_profile = convertApplicationProfile(fb_cp->application_profile());
        result.depends_on = convertStringVector(fb_cp->depends_on());
        result.process_arguments = convertStringVector(fb_cp->process_arguments());
        result.ready_condition = convertReadyCondition(fb_cp->ready_condition());
    }
    return result;
}

score::cpp::expected<std::optional<Sandbox>, IConfigLoader::Error> convertSandbox(const fb::Sandbox* fb_sb)
{
    if (fb_sb == nullptr)
    {
        return std::nullopt;
    }
    const auto fb_uid = fb_sb->uid();
    const auto fb_gid = fb_sb->gid();
    if (fb_uid < std::numeric_limits<uid_t>::min() || fb_uid > std::numeric_limits<uid_t>::max())
    {
        LM_LOG_ERROR() << "Sandbox uid " << fb_uid << " is out of valid uid_t range [" << std::numeric_limits<uid_t>::min() << "," << std::numeric_limits<uid_t>::max() << "]";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    if (fb_gid < std::numeric_limits<gid_t>::min() || fb_gid > std::numeric_limits<gid_t>::max())
    {
        LM_LOG_ERROR() << "Sandbox gid " << fb_gid << " is out of valid gid_t range [" << std::numeric_limits<gid_t>::min() << "," << std::numeric_limits<gid_t>::max() << "]";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    Sandbox result{};
    result.uid = static_cast<uid_t>(fb_uid);
    result.gid = static_cast<gid_t>(fb_gid);
    auto supplementary_gids = convertGidVector(fb_sb->supplementary_group_ids());
    if (!supplementary_gids.has_value())
    {
        return score::cpp::make_unexpected(supplementary_gids.error());
    }
    result.supplementary_group_ids = std::move(*supplementary_gids);
    result.security_policy = fb_sb->security_policy() != nullptr
                                 ? std::optional<std::string>{fb_sb->security_policy()->str()}
                                 : std::nullopt;
    result.scheduling_policy = safeString(fb_sb->scheduling_policy());
    result.scheduling_priority = fb_sb->scheduling_priority().value_or(0);
    result.max_memory_usage =
        fb_sb->max_memory_usage().has_value() ? std::optional<uint64_t>{*fb_sb->max_memory_usage()} : std::nullopt;
    result.max_cpu_usage =
        fb_sb->max_cpu_usage().has_value() ? std::optional<uint32_t>{*fb_sb->max_cpu_usage()} : std::nullopt;
    return std::optional<Sandbox>{std::move(result)};
}

score::cpp::expected<DeploymentConfig, IConfigLoader::Error> convertDeploymentConfig(const fb::DeploymentConfig* fb_dc)
{
    DeploymentConfig result{};
    if (fb_dc != nullptr)
    {
        assert(fb_dc->bin_dir() && "DeploymentConfig::bin_dir must never be nullptr as it is required in the schema");
        result.ready_timeout_ms = secondsToMs(fb_dc->ready_timeout());
        result.shutdown_timeout_ms = secondsToMs(fb_dc->shutdown_timeout());
        result.environmental_variables = convertEnvironmentalVariables(fb_dc->environmental_variables());
        result.bin_dir = fb_dc->bin_dir()->str();
        result.working_dir = safeString(fb_dc->working_dir());
        result.ready_recovery_action = convertRestartAction(fb_dc->ready_recovery_action());
        result.recovery_action = convertSwitchRunTargetAction(fb_dc->recovery_action());
        auto sandbox = convertSandbox(fb_dc->sandbox());
        if (!sandbox.has_value())
        {
            return score::cpp::make_unexpected(sandbox.error());
        }
        result.sandbox = std::move(*sandbox);
    }
    return result;
}

score::cpp::expected<ComponentConfig, IConfigLoader::Error> convertComponent(const fb::Component* fb_comp)
{
    ComponentConfig result{};
    if (fb_comp != nullptr)
    {
        assert(fb_comp->name() && "Component::name must never be nullptr as it is required in the schema");
        assert(fb_comp->component_properties() && "Component::component_properties must never be nullptr as it is required in the schema");
        assert(fb_comp->deployment_config() && "Component::deployment_config must never be nullptr as it is required in the schema");
        result.name = fb_comp->name()->str();
        result.description = safeString(fb_comp->description());
        result.component_properties = convertComponentProperties(fb_comp->component_properties());
        auto deployment_config = convertDeploymentConfig(fb_comp->deployment_config());
        if (!deployment_config.has_value())
        {
            return score::cpp::make_unexpected(deployment_config.error());
        }
        result.deployment_config = std::move(*deployment_config);
    }
    return result;
}

RunTargetConfig convertRunTarget(const fb::RunTarget* fb_rt)
{
    RunTargetConfig result{};
    if (fb_rt != nullptr)
    {
        assert(fb_rt->name() && "RunTarget::name must never be nullptr as it is required in the schema");
        assert(fb_rt->recovery_action() && "RunTarget::recovery_action must never be nullptr as it is required in the schema");
        result.name = fb_rt->name()->str();
        result.description = safeString(fb_rt->description());
        result.depends_on = convertStringVector(fb_rt->depends_on());
        result.transition_timeout_ms = secondsToMs(fb_rt->transition_timeout());
        result.recovery_action = convertRequiredSwitchRunTargetAction(fb_rt->recovery_action());
    }
    return result;
}

FallbackRunTargetConfig convertFallbackRunTarget(const fb::FallbackRunTarget* fb_frt)
{
    FallbackRunTargetConfig result{};
    if (fb_frt != nullptr)
    {
        result.description = safeString(fb_frt->description());
        result.depends_on = convertStringVector(fb_frt->depends_on());
        result.transition_timeout_ms = secondsToMs(fb_frt->transition_timeout());
    }
    return result;
}

AliveSupervisionConfig convertAliveSupervision(const fb::AliveSupervision* fb_as)
{
    if (fb_as == nullptr)
    {
        return AliveSupervisionConfig{};
    }
    return AliveSupervisionConfig{secondsToMs(fb_as->evaluation_cycle())};
}

std::optional<WatchdogConfig> convertWatchdog(const fb::Watchdog* fb_wd)
{
    if (fb_wd == nullptr)
    {
        return std::nullopt;
    }
    assert(fb_wd->device_file_path() && "Watchdog::device_file_path must never be nullptr as it is required in the schema");
    WatchdogConfig result{};
    result.device_file_path = fb_wd->device_file_path()->str();
    result.max_timeout_ms = secondsToMs(fb_wd->max_timeout());
    result.deactivate_on_shutdown =
        fb_wd->deactivate_on_shutdown().has_value() ? *fb_wd->deactivate_on_shutdown() : false;
    result.require_magic_close = fb_wd->require_magic_close().has_value() ? *fb_wd->require_magic_close() : false;
    return result;
}

}  // anonymous namespace

namespace details
{

// --- File I/O error mapping ---

IConfigLoader::Error mapOsError(const score::os::Error& error)
{
    if (error == score::os::Error::Code::kNoSuchFileOrDirectory)
    {
        return IConfigLoader::Error::FileNotFound;
    }
    if (error == score::os::Error::Code::kPermissionDenied)
    {
        return IConfigLoader::Error::InsufficientPermission;
    }
    return IConfigLoader::Error::GeneralError;
}

// --- FlatBuffer parsing and conversion ---

score::cpp::expected<Config, IConfigLoader::Error> parseFlatbuffer(const std::vector<uint8_t>& buffer)
{
    ::flatbuffers::Verifier verifier(buffer.data(), buffer.size());
    if (!fb::VerifyLaunchManagerConfigBuffer(verifier))
    {
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }

    const auto* config = fb::GetLaunchManagerConfig(buffer.data());
    if (config == nullptr)
    {
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }

    if (config->schema_version() != kExpectedSchemaVersion)
    {
        return score::cpp::make_unexpected(IConfigLoader::Error::UnsupportedVersion);
    }

    ConfigBuilder builder;

    assert(config->initial_run_target() && "LaunchManagerConfig::initial_run_target must never be nullptr as it is required in the schema");
    builder.setInitialRunTarget(config->initial_run_target()->str());

    if (config->components() != nullptr)
    {
        std::vector<ComponentConfig> components;
        components.reserve(config->components()->size());
        for (const auto* comp : *config->components())
        {
            if (comp != nullptr)
            {
                auto component = convertComponent(comp);
                if (!component.has_value())
                {
                    return score::cpp::make_unexpected(component.error());
                }
                components.emplace_back(std::move(*component));
            }
        }
        builder.setComponents(std::move(components));
    }

    if (config->run_targets() != nullptr)
    {
        std::vector<RunTargetConfig> run_targets;
        run_targets.reserve(config->run_targets()->size());
        for (const auto* rt : *config->run_targets())
        {
            if (rt != nullptr)
            {
                run_targets.emplace_back(convertRunTarget(rt));
            }
        }
        builder.setRunTargets(std::move(run_targets));
    }

    assert(config->fallback_run_target() && "LaunchManagerConfig::fallback_run_target must never be nullptr as it is required in the schema");
    builder.setFallbackRunTarget(convertFallbackRunTarget(config->fallback_run_target()));

    assert(config->alive_supervision() && "LaunchManagerConfig::alive_supervision must never be nullptr as it is required in the schema");
    builder.setAliveSupervision(convertAliveSupervision(config->alive_supervision()));

    auto wd = convertWatchdog(config->watchdog());
    if (wd.has_value())
    {
        builder.setWatchdog(std::move(*wd));
    }

    return builder.build();
}

}  // namespace details
}  // namespace score::launch_manager::config
