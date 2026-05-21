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

#include <cstdint>
#include <string>
#include <unordered_map>
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
    return static_cast<uint32_t>(seconds * kSecondsToMilliseconds);
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

std::vector<uint32_t> convertUint32Vector(const ::flatbuffers::Vector<uint32_t>* vec)
{
    std::vector<uint32_t> result;
    if (vec != nullptr)
    {
        result.reserve(vec->size());
        for (const auto val : *vec)
        {
            result.emplace_back(val);
        }
    }
    return result;
}

std::unordered_map<std::string, std::string> convertEnvironmentalVariables(
    const ::flatbuffers::Vector<::flatbuffers::Offset<fb::EnvironmentalVariable>>* vec)
{
    std::unordered_map<std::string, std::string> result;
    if (vec != nullptr)
    {
        result.reserve(vec->size());
        for (const auto* ev : *vec)
        {
            if (ev != nullptr && ev->key() != nullptr)
            {
                result.emplace(ev->key()->str(), safeString(ev->value()));
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
    return SwitchRunTargetAction{safeString(sa->run_target())};
}

SwitchRunTargetAction convertRequiredSwitchRunTargetAction(const fb::SwitchRunTargetAction* sa)
{
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
        result.binary_name = safeString(fb_cp->binary_name());
        result.application_profile = convertApplicationProfile(fb_cp->application_profile());
        result.depends_on = convertStringVector(fb_cp->depends_on());
        result.process_arguments = convertStringVector(fb_cp->process_arguments());
        result.ready_condition = convertReadyCondition(fb_cp->ready_condition());
    }
    return result;
}

std::optional<Sandbox> convertSandbox(const fb::Sandbox* fb_sb)
{
    if (fb_sb == nullptr)
    {
        return std::nullopt;
    }
    Sandbox result{};
    result.uid = fb_sb->uid();
    result.gid = fb_sb->gid();
    result.supplementary_group_ids = convertUint32Vector(fb_sb->supplementary_group_ids());
    result.security_policy = fb_sb->security_policy() != nullptr
                                 ? std::optional<std::string>{fb_sb->security_policy()->str()}
                                 : std::nullopt;
    result.scheduling_policy = safeString(fb_sb->scheduling_policy());
    result.scheduling_priority = fb_sb->scheduling_priority().value_or(0);
    result.max_memory_usage =
        fb_sb->max_memory_usage().has_value() ? std::optional<uint64_t>{*fb_sb->max_memory_usage()} : std::nullopt;
    result.max_cpu_usage =
        fb_sb->max_cpu_usage().has_value() ? std::optional<uint32_t>{*fb_sb->max_cpu_usage()} : std::nullopt;
    return result;
}

DeploymentConfig convertDeploymentConfig(const fb::DeploymentConfig* fb_dc)
{
    DeploymentConfig result{};
    if (fb_dc != nullptr)
    {
        result.ready_timeout_ms = secondsToMs(fb_dc->ready_timeout());
        result.shutdown_timeout_ms = secondsToMs(fb_dc->shutdown_timeout());
        result.environmental_variables = convertEnvironmentalVariables(fb_dc->environmental_variables());
        result.bin_dir = safeString(fb_dc->bin_dir());
        result.working_dir = safeString(fb_dc->working_dir());
        result.ready_recovery_action = convertRestartAction(fb_dc->ready_recovery_action());
        result.recovery_action = convertSwitchRunTargetAction(fb_dc->recovery_action());
        result.sandbox = convertSandbox(fb_dc->sandbox());
    }
    return result;
}

ComponentConfig convertComponent(const fb::Component* fb_comp)
{
    ComponentConfig result{};
    if (fb_comp != nullptr)
    {
        result.name = safeString(fb_comp->name());
        result.description = safeString(fb_comp->description());
        result.component_properties = convertComponentProperties(fb_comp->component_properties());
        result.deployment_config = convertDeploymentConfig(fb_comp->deployment_config());
    }
    return result;
}

RunTargetConfig convertRunTarget(const fb::RunTarget* fb_rt)
{
    RunTargetConfig result{};
    if (fb_rt != nullptr)
    {
        result.name = safeString(fb_rt->name());
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
    WatchdogConfig result{};
    result.device_file_path = safeString(fb_wd->device_file_path());
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

    // initial_run_target is a required field, guaranteed non-null by the schema and verifier.
    builder.setInitialRunTarget(config->initial_run_target()->str());

    if (config->components() != nullptr)
    {
        std::vector<ComponentConfig> components;
        components.reserve(config->components()->size());
        for (const auto* comp : *config->components())
        {
            if (comp != nullptr)
            {
                components.emplace_back(convertComponent(comp));
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

    // fallback_run_target is a required field, guaranteed non-null by the schema and verifier.
    builder.setFallbackRunTarget(convertFallbackRunTarget(config->fallback_run_target()));

    // alive_supervision is a required field, guaranteed non-null by the schema and verifier.
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
