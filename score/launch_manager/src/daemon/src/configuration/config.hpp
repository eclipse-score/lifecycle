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

#include <sys/types.h>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "score/mw/launch_manager/configuration/component_config.hpp"

namespace score::mw::lifecycle::internal::configuration
{

struct RunTargetConfig
{
    std::string name;
    std::string description;
    std::vector<std::string> depends_on;
    std::uint32_t transition_timeout_ms{};
    SwitchRunTargetAction recovery_action;
};

struct FallbackRunTargetConfig
{
    std::string description;
    std::vector<std::string> depends_on;
    std::uint32_t transition_timeout_ms{};
};

/// @brief Config members needed to build the graph
struct GraphConfig
{
    /// @brief Components that Run Targets may depend on
    std::vector<ComponentConfig> components_;
    /// @brief Run Targets that can be activated
    std::vector<RunTargetConfig> run_targets_;
    /// @brief Information about the Run Target transitioned to in the event of an error
    FallbackRunTargetConfig fallback_run_target_;
    /// @brief Name of the first Run Target to launch
    std::string initial_run_target_;
};

struct AliveSupervisionConfig
{
    std::uint32_t evaluation_cycle_ms{};
};

class ConfigBuilder;

/// @brief Move-only value object holding the parsed launch-manager configuration.
///
/// Loaded once by an IConfigLoader, then individual parts are moved out via
/// the `take*()` accessors to transfer ownership to dedicated domain objects
/// (e.g. ComponentConfig to Component).
///
/// @note As of now, everything is expected in a single config file.
/// Even though some fields appear as optional in the json schema, they are only optional if the configuration would be
/// split across multiple files. As only a single file is supported, the single config will contain all fields.
struct Config
{
    Config(const Config&) = delete;
    Config(Config&&) noexcept = default;

    Config& operator=(const Config&) = delete;
    Config& operator=(Config&&) noexcept = default;
    ~Config() noexcept = default;

    /// @brief Returns the configuration needed to build the graph (components and Run Targets).
    [[nodiscard]] const GraphConfig& graph() const;
    /// @brief Returns the global alive supervision configuration.
    [[nodiscard]] const AliveSupervisionConfig& aliveSupervision() const;

    /// @brief Moves out the configuration needed to build the graph. Source is left in a moved-from state.
    [[nodiscard]] GraphConfig takeGraph();
    /// @brief Moves out the alive supervision configuration. Source is left in a moved-from state.
    [[nodiscard]] AliveSupervisionConfig takeAliveSupervision();

  private:
    friend class ConfigBuilder;

    Config(GraphConfig graph, AliveSupervisionConfig alive_supervision);

    GraphConfig graph_;
    AliveSupervisionConfig alive_supervision_;
};

/// @brief Builder for constructing a @ref Config instance.
class ConfigBuilder
{
  public:
    /// @brief Sets the component configurations.
    ConfigBuilder& setComponents(std::vector<ComponentConfig> components);
    /// @brief Sets the Run Target configurations.
    ConfigBuilder& setRunTargets(std::vector<RunTargetConfig> run_targets);
    /// @brief Sets the initial Run Target name.
    ConfigBuilder& setInitialRunTarget(std::string initial_run_target);
    /// @brief Sets the fallback Run Target configuration.
    ConfigBuilder& setFallbackRunTarget(FallbackRunTargetConfig fallback);
    /// @brief Sets the global alive supervision configuration.
    ConfigBuilder& setAliveSupervision(AliveSupervisionConfig alive_supervision);

    /// @brief Builds and returns the @ref Config object.
    Config build();

  private:
    std::string initial_run_target_;
    std::vector<ComponentConfig> components_;
    std::vector<RunTargetConfig> run_targets_;
    FallbackRunTargetConfig fallback_run_target_;
    AliveSupervisionConfig alive_supervision_;
};

}  // namespace score::mw::lifecycle::internal::configuration

#endif  // CONFIG_HPP
