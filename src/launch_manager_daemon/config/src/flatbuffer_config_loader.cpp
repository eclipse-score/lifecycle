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
#include <sched.h>
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

score::cpp::expected<int32_t, IConfigLoader::Error> convertSchedulingPolicy(fb::SchedulingPolicy policy)
{
    switch (policy)
    {
        case fb::SchedulingPolicy::OTHER:
            return SCHED_OTHER;
        case fb::SchedulingPolicy::FIFO:
            return SCHED_FIFO;
        case fb::SchedulingPolicy::RR:
            return SCHED_RR;
        default:
            LM_LOG_ERROR() << "Unsupported scheduling policy: " << static_cast<int>(policy);
            return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
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

score::cpp::expected<std::optional<RestartAction>, IConfigLoader::Error> convertRestartAction(
    const fb::RestartAction* ra)
{
    if (ra == nullptr)
    {
        return std::optional<RestartAction>{std::nullopt};
    }
    if (!ra->number_of_attempts().has_value())
    {
        LM_LOG_ERROR() << "RestartAction::number_of_attempts is required but missing";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    if (!ra->delay_before_restart().has_value())
    {
        LM_LOG_ERROR() << "RestartAction::delay_before_restart is required but missing";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    return std::optional<RestartAction>{
        RestartAction{*ra->number_of_attempts(), secondsToMs(*ra->delay_before_restart())}};
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

score::cpp::expected<ComponentAliveSupervision, IConfigLoader::Error> convertComponentAliveSupervision(
    const fb::ComponentAliveSupervision* fb_cas)
{
    ComponentAliveSupervision result{};
    if (fb_cas != nullptr)
    {
        if (!fb_cas->reporting_cycle().has_value())
        {
            LM_LOG_ERROR() << "ComponentAliveSupervision::reporting_cycle is required but missing";
            return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
        }
        if (!fb_cas->failed_cycles_tolerance().has_value())
        {
            LM_LOG_ERROR() << "ComponentAliveSupervision::failed_cycles_tolerance is required but missing";
            return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
        }
        result.reporting_cycle_ms = secondsToMs(*fb_cas->reporting_cycle());
        result.failed_cycles_tolerance = *fb_cas->failed_cycles_tolerance();
        result.min_indications = fb_cas->min_indications().has_value()
                                     ? std::optional<uint32_t>{*fb_cas->min_indications()}
                                     : std::nullopt;
        result.max_indications = fb_cas->max_indications().has_value()
                                     ? std::optional<uint32_t>{*fb_cas->max_indications()}
                                     : std::nullopt;
    }
    return result;
}

score::cpp::expected<ApplicationProfile, IConfigLoader::Error> convertApplicationProfile(
    const fb::ApplicationProfile* fb_ap)
{
    ApplicationProfile result{};
    if (fb_ap != nullptr)
    {
        if (!fb_ap->application_type().has_value())
        {
            LM_LOG_ERROR() << "ApplicationProfile::application_type is required but missing";
            return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
        }
        if (!fb_ap->is_self_terminating().has_value())
        {
            LM_LOG_ERROR() << "ApplicationProfile::is_self_terminating is required but missing";
            return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
        }
        result.application_type = convertApplicationType(*fb_ap->application_type());
        result.is_self_terminating = *fb_ap->is_self_terminating();
        if (fb_ap->alive_supervision() != nullptr)
        {
            auto alive_sup = convertComponentAliveSupervision(fb_ap->alive_supervision());
            if (!alive_sup.has_value())
            {
                return score::cpp::make_unexpected(alive_sup.error());
            }
            result.alive_supervision = std::move(*alive_sup);
        }
    }
    return result;
}

score::cpp::expected<ReadyCondition, IConfigLoader::Error> convertReadyCondition(const fb::ReadyCondition* fb_rc)
{
    ReadyCondition result{};
    if (fb_rc != nullptr)
    {
        if (!fb_rc->process_state().has_value())
        {
            LM_LOG_ERROR() << "ReadyCondition::process_state is required but missing";
            return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
        }
        result.process_state = convertProcessState(*fb_rc->process_state());
    }
    return result;
}

score::cpp::expected<ComponentProperties, IConfigLoader::Error> convertComponentProperties(
    const fb::ComponentProperties* fb_cp)
{
    ComponentProperties result{};
    if (fb_cp != nullptr)
    {
        assert(fb_cp->binary_name() && "ComponentProperties::binary_name must never be nullptr as it is required in the schema");
        assert(fb_cp->application_profile() && "ComponentProperties::application_profile must never be nullptr as it is required in the schema");
        result.binary_name = fb_cp->binary_name()->str();
        auto app_profile = convertApplicationProfile(fb_cp->application_profile());
        if (!app_profile.has_value())
        {
            return score::cpp::make_unexpected(app_profile.error());
        }
        result.application_profile = std::move(*app_profile);
        result.depends_on = convertStringVector(fb_cp->depends_on());
        result.process_arguments = convertStringVector(fb_cp->process_arguments());
        if (fb_cp->ready_condition() != nullptr)
        {
            auto ready_cond = convertReadyCondition(fb_cp->ready_condition());
            if (!ready_cond.has_value())
            {
                return score::cpp::make_unexpected(ready_cond.error());
            }
            result.ready_condition = std::move(*ready_cond);
        }
    }
    return result;
}

score::cpp::expected<Sandbox, IConfigLoader::Error> convertSandbox(const fb::Sandbox* fb_sb)
{
    assert(fb_sb && "Sandbox must never be nullptr as it is required in the schema");
    if (!fb_sb->uid().has_value())
    {
        LM_LOG_ERROR() << "Sandbox::uid is required but missing";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    if (!fb_sb->gid().has_value())
    {
        LM_LOG_ERROR() << "Sandbox::gid is required but missing";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    const auto fb_uid = *fb_sb->uid();
    const auto fb_gid = *fb_sb->gid();
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
    if (!fb_sb->scheduling_policy().has_value())
    {
        LM_LOG_ERROR() << "Sandbox::scheduling_policy is required but missing";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    auto scheduling_policy = convertSchedulingPolicy(*fb_sb->scheduling_policy());
    if (!scheduling_policy.has_value())
    {
        return score::cpp::make_unexpected(scheduling_policy.error());
    }
    if (!fb_sb->scheduling_priority().has_value())
    {
        LM_LOG_ERROR() << "Sandbox::scheduling_priority is required but missing";
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
    result.scheduling_policy = *scheduling_policy;
    result.scheduling_priority = *fb_sb->scheduling_priority();
    result.max_memory_usage =
        fb_sb->max_memory_usage().has_value() ? std::optional<uint64_t>{*fb_sb->max_memory_usage()} : std::nullopt;
    result.max_cpu_usage =
        fb_sb->max_cpu_usage().has_value() ? std::optional<uint32_t>{*fb_sb->max_cpu_usage()} : std::nullopt;
    return result;
}

score::cpp::expected<DeploymentConfig, IConfigLoader::Error> convertDeploymentConfig(const fb::DeploymentConfig* fb_dc)
{
    DeploymentConfig result{};
    if (fb_dc != nullptr)
    {
        assert(fb_dc->bin_dir() && "DeploymentConfig::bin_dir must never be nullptr as it is required in the schema");
        assert(fb_dc->working_dir() && "DeploymentConfig::working_dir must never be nullptr as it is required in the schema");
        assert(fb_dc->sandbox() && "DeploymentConfig::sandbox must never be nullptr as it is required in the schema");
        if (!fb_dc->ready_timeout().has_value())
        {
            LM_LOG_ERROR() << "DeploymentConfig::ready_timeout is required but missing";
            return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
        }
        if (!fb_dc->shutdown_timeout().has_value())
        {
            LM_LOG_ERROR() << "DeploymentConfig::shutdown_timeout is required but missing";
            return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
        }
        result.ready_timeout_ms = secondsToMs(*fb_dc->ready_timeout());
        result.shutdown_timeout_ms = secondsToMs(*fb_dc->shutdown_timeout());
        result.environmental_variables = convertEnvironmentalVariables(fb_dc->environmental_variables());
        result.bin_dir = fb_dc->bin_dir()->str();
        result.working_dir = fb_dc->working_dir()->str();
        auto ready_recovery = convertRestartAction(fb_dc->ready_recovery_action());
        if (!ready_recovery.has_value())
        {
            return score::cpp::make_unexpected(ready_recovery.error());
        }
        result.ready_recovery_action = std::move(*ready_recovery);
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
        auto component_properties = convertComponentProperties(fb_comp->component_properties());
        if (!component_properties.has_value())
        {
            return score::cpp::make_unexpected(component_properties.error());
        }
        result.component_properties = std::move(*component_properties);
        auto deployment_config = convertDeploymentConfig(fb_comp->deployment_config());
        if (!deployment_config.has_value())
        {
            return score::cpp::make_unexpected(deployment_config.error());
        }
        result.deployment_config = std::move(*deployment_config);
    }
    return result;
}

score::cpp::expected<RunTargetConfig, IConfigLoader::Error> convertRunTarget(const fb::RunTarget* fb_rt)
{
    RunTargetConfig result{};
    if (fb_rt != nullptr)
    {
        assert(fb_rt->name() && "RunTarget::name must never be nullptr as it is required in the schema");
        assert(fb_rt->recovery_action() && "RunTarget::recovery_action must never be nullptr as it is required in the schema");
        if (!fb_rt->transition_timeout().has_value())
        {
            LM_LOG_ERROR() << "RunTarget::transition_timeout is required but missing";
            return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
        }
        result.name = fb_rt->name()->str();
        result.description = safeString(fb_rt->description());
        result.depends_on = convertStringVector(fb_rt->depends_on());
        result.transition_timeout_ms = secondsToMs(*fb_rt->transition_timeout());
        result.recovery_action = convertRequiredSwitchRunTargetAction(fb_rt->recovery_action());
    }
    return result;
}

score::cpp::expected<FallbackRunTargetConfig, IConfigLoader::Error> convertFallbackRunTarget(
    const fb::FallbackRunTarget* fb_frt)
{
    FallbackRunTargetConfig result{};
    if (fb_frt != nullptr)
    {
        if (!fb_frt->transition_timeout().has_value())
        {
            LM_LOG_ERROR() << "FallbackRunTarget::transition_timeout is required but missing";
            return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
        }
        result.description = safeString(fb_frt->description());
        result.depends_on = convertStringVector(fb_frt->depends_on());
        result.transition_timeout_ms = secondsToMs(*fb_frt->transition_timeout());
    }
    return result;
}

score::cpp::expected<AliveSupervisionConfig, IConfigLoader::Error> convertAliveSupervision(
    const fb::AliveSupervision* fb_as)
{
    if (fb_as == nullptr)
    {
        return AliveSupervisionConfig{};
    }
    if (!fb_as->evaluation_cycle().has_value())
    {
        LM_LOG_ERROR() << "AliveSupervision::evaluation_cycle is required but missing";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    return AliveSupervisionConfig{secondsToMs(*fb_as->evaluation_cycle())};
}

score::cpp::expected<std::optional<WatchdogConfig>, IConfigLoader::Error> convertWatchdog(const fb::Watchdog* fb_wd)
{
    if (fb_wd == nullptr)
    {
        return std::optional<WatchdogConfig>{std::nullopt};
    }
    assert(fb_wd->device_file_path() && "Watchdog::device_file_path must never be nullptr as it is required in the schema");
    if (!fb_wd->max_timeout().has_value())
    {
        LM_LOG_ERROR() << "Watchdog::max_timeout is required but missing";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    if (!fb_wd->deactivate_on_shutdown().has_value())
    {
        LM_LOG_ERROR() << "Watchdog::deactivate_on_shutdown is required but missing";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    if (!fb_wd->require_magic_close().has_value())
    {
        LM_LOG_ERROR() << "Watchdog::require_magic_close is required but missing";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    WatchdogConfig result{};
    result.device_file_path = fb_wd->device_file_path()->str();
    result.max_timeout_ms = secondsToMs(*fb_wd->max_timeout());
    result.deactivate_on_shutdown = *fb_wd->deactivate_on_shutdown();
    result.require_magic_close = *fb_wd->require_magic_close();
    return std::optional<WatchdogConfig>{std::move(result)};
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

    if (!config->schema_version().has_value())
    {
        LM_LOG_ERROR() << "LaunchManagerConfig::schema_version is required but missing";
        return score::cpp::make_unexpected(IConfigLoader::Error::InvalidFormat);
    }
    if (*config->schema_version() != kExpectedSchemaVersion)
    {
        return score::cpp::make_unexpected(IConfigLoader::Error::UnsupportedVersion);
    }

    ConfigBuilder builder;

    assert(config->initial_run_target() && "LaunchManagerConfig::initial_run_target must never be nullptr as it is required in the schema");
    assert(config->components() && "LaunchManagerConfig::components must never be nullptr as it is required in the schema");
    assert(config->run_targets() && "LaunchManagerConfig::run_targets must never be nullptr as it is required in the schema");
    builder.setInitialRunTarget(config->initial_run_target()->str());

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

    {
        std::vector<RunTargetConfig> run_targets;
        run_targets.reserve(config->run_targets()->size());
        for (const auto* rt : *config->run_targets())
        {
            if (rt != nullptr)
            {
                auto run_target = convertRunTarget(rt);
                if (!run_target.has_value())
                {
                    return score::cpp::make_unexpected(run_target.error());
                }
                run_targets.emplace_back(std::move(*run_target));
            }
        }
        builder.setRunTargets(std::move(run_targets));
    }

    assert(config->fallback_run_target() && "LaunchManagerConfig::fallback_run_target must never be nullptr as it is required in the schema");
    auto fallback = convertFallbackRunTarget(config->fallback_run_target());
    if (!fallback.has_value())
    {
        return score::cpp::make_unexpected(fallback.error());
    }
    builder.setFallbackRunTarget(std::move(*fallback));

    assert(config->alive_supervision() && "LaunchManagerConfig::alive_supervision must never be nullptr as it is required in the schema");
    auto alive_sup = convertAliveSupervision(config->alive_supervision());
    if (!alive_sup.has_value())
    {
        return score::cpp::make_unexpected(alive_sup.error());
    }
    builder.setAliveSupervision(std::move(*alive_sup));

    auto wd = convertWatchdog(config->watchdog());
    if (!wd.has_value())
    {
        return score::cpp::make_unexpected(wd.error());
    }
    if (wd->has_value())
    {
        builder.setWatchdog(std::move(**wd));
    }

    return builder.build();
}

}  // namespace details
}  // namespace score::launch_manager::config
