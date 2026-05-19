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
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace score::launch_manager::config
{

enum class ApplicationType : uint8_t
{
    Native = 0,
    Reporting = 1,
    ReportingAndSupervised = 2,
    StateManager = 3
};

enum class ProcessState : uint8_t
{
    Running = 0,
    Terminated = 1
};

struct ComponentAliveSupervision
{
    uint32_t reporting_cycle_ms{};
    uint32_t failed_cycles_tolerance{};
    uint32_t min_indications{};
    uint32_t max_indications{};
};

struct ApplicationProfile
{
    ApplicationType application_type{ApplicationType::Native};
    bool is_self_terminating{};
    std::optional<ComponentAliveSupervision> alive_supervision;
};

struct ReadyCondition
{
    ProcessState process_state{ProcessState::Running};
};

struct ComponentProperties
{
    std::string binary_name;
    ApplicationProfile application_profile;
    std::vector<std::string> depends_on;
    std::vector<std::string> process_arguments;
    ReadyCondition ready_condition;
};

struct RestartAction
{
    uint32_t number_of_attempts{};
    uint32_t delay_before_restart_ms{};
};

struct SwitchRunTargetAction
{
    std::string run_target;
};

struct Sandbox
{
    uint32_t uid{};
    uint32_t gid{};
    std::vector<uint32_t> supplementary_group_ids;
    std::optional<std::string> security_policy;
    std::string scheduling_policy;
    int32_t scheduling_priority{};
    std::optional<uint64_t> max_memory_usage;
    std::optional<uint32_t> max_cpu_usage;
};

struct DeploymentConfig
{
    uint32_t ready_timeout_ms{};
    uint32_t shutdown_timeout_ms{};
    std::unordered_map<std::string, std::string> environmental_variables;
    std::string bin_dir;
    std::string working_dir;
    std::optional<RestartAction> ready_recovery_action;
    // Currently only SwitchRunTargetAction is supported here, RestartAction to be added in the future
    std::optional<SwitchRunTargetAction> recovery_action;
    std::optional<Sandbox> sandbox;
};

struct ComponentConfig
{
    std::string name;
    std::string description;
    ComponentProperties component_properties;
    DeploymentConfig deployment_config;
};

struct RunTargetConfig
{
    std::string name;
    std::string description;
    std::vector<std::string> depends_on;
    uint32_t transition_timeout_ms{};
    SwitchRunTargetAction recovery_action;
};

struct FallbackRunTargetConfig
{
    std::string description;
    std::vector<std::string> depends_on;
    uint32_t transition_timeout_ms{};
};

struct AliveSupervisionConfig
{
    uint32_t evaluation_cycle_ms{};
};

struct WatchdogConfig
{
    std::string device_file_path;
    uint32_t max_timeout_ms{};
    bool deactivate_on_shutdown{};
    bool require_magic_close{};
};

/// @brief Move-only value object holding the parsed launch-manager configuration.
///
/// Loaded once by an IConfigLoader, then individual parts are moved out via
/// the `take_*()` accessors to transfer ownership to dedicated domain objects
/// (e.g. ComponentConfig to Component).
///
/// @note As of now, everything is expected in a single config file.
/// Even though some fields appear as optional in the json schema, they are only optional if the configuration would be
/// split across multiple files. As only a single file is supported, the single config will contain all fields.
class Config
{
  public:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = default;
    Config& operator=(Config&&) = default;

    class Builder
    {
      public:
        Builder& setComponents(std::vector<ComponentConfig> components);
        Builder& setRunTargets(std::vector<RunTargetConfig> run_targets);
        Builder& setInitialRunTarget(std::string initial_run_target);
        Builder& setFallbackRunTarget(FallbackRunTargetConfig fallback);
        Builder& setAliveSupervision(AliveSupervisionConfig alive_supervision);
        Builder& setWatchdog(WatchdogConfig watchdog);

        Config build();

      private:
        std::string initial_run_target_;
        std::vector<ComponentConfig> components_;
        std::vector<RunTargetConfig> run_targets_;
        FallbackRunTargetConfig fallback_run_target_;
        AliveSupervisionConfig alive_supervision_;
        std::optional<WatchdogConfig> watchdog_;
    };

    // Read access
    const std::vector<ComponentConfig>& components() const
    {
        return components_;
    }
    const std::vector<RunTargetConfig>& run_targets() const
    {
        return run_targets_;
    }
    std::string_view initial_run_target() const
    {
        return initial_run_target_;
    }
    const FallbackRunTargetConfig& fallback_run_target() const
    {
        return fallback_run_target_;
    }
    const AliveSupervisionConfig& alive_supervision() const
    {
        return alive_supervision_;
    }
    const std::optional<WatchdogConfig>& watchdog() const
    {
        return watchdog_;
    }

    // Ownership transfer — source is left in a moved-from state after the call
    std::vector<ComponentConfig> take_components()
    {
        return std::move(components_);
    }
    std::vector<RunTargetConfig> take_run_targets()
    {
        return std::move(run_targets_);
    }
    std::string take_initial_run_target()
    {
        return std::move(initial_run_target_);
    }
    FallbackRunTargetConfig take_fallback_run_target()
    {
        return std::move(fallback_run_target_);
    }
    AliveSupervisionConfig take_alive_supervision()
    {
        return std::move(alive_supervision_);
    }
    std::optional<WatchdogConfig> take_watchdog()
    {
        return std::move(watchdog_);
    }

  private:
    friend class Builder;

    Config(std::vector<ComponentConfig> components,
           std::vector<RunTargetConfig> run_targets,
           std::string initial_run_target,
           FallbackRunTargetConfig fallback_run_target,
           AliveSupervisionConfig alive_supervision,
           std::optional<WatchdogConfig> watchdog);

    std::vector<ComponentConfig> components_;
    std::vector<RunTargetConfig> run_targets_;
    std::string initial_run_target_;
    FallbackRunTargetConfig fallback_run_target_;
    AliveSupervisionConfig alive_supervision_;
    std::optional<WatchdogConfig> watchdog_;
};

}  // namespace score::launch_manager::config

#endif
