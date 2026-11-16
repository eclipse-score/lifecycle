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
#include "score/mw/launch_manager/configuration/config.hpp"

#include <score/assert.hpp>

#include <utility>

namespace score::mw::lifecycle::internal::configuration
{

// --- Config ---

Config::Config(GraphConfig graph, AliveSupervisionConfig alive_supervision)
    : graph_{std::move(graph)}, alive_supervision_{alive_supervision}
{
}

ConfigBuilder& ConfigBuilder::setComponents(std::vector<ComponentConfig> components)
{
    components_ = std::move(components);
    return *this;
}

ConfigBuilder& ConfigBuilder::setRunTargets(std::vector<RunTargetConfig> run_targets)
{
    run_targets_ = std::move(run_targets);
    return *this;
}

ConfigBuilder& ConfigBuilder::setInitialRunTarget(std::string initial_run_target)
{
    initial_run_target_ = std::move(initial_run_target);
    return *this;
}

ConfigBuilder& ConfigBuilder::setFallbackRunTarget(FallbackRunTargetConfig fallback)
{
    fallback_run_target_ = std::move(fallback);
    return *this;
}

ConfigBuilder& ConfigBuilder::setAliveSupervision(AliveSupervisionConfig alive_supervision)
{
    alive_supervision_ = std::move(alive_supervision);
    return *this;
}

Config ConfigBuilder::build()
{
    return Config(
        GraphConfig{
            std::move(components_),
            std::move(run_targets_),
            std::move(fallback_run_target_),
            std::move(initial_run_target_)},
        std::move(alive_supervision_));
}

const GraphConfig& Config::graph() const
{
    return graph_;
}

const AliveSupervisionConfig& Config::aliveSupervision() const
{
    return alive_supervision_;
}

GraphConfig Config::takeGraph()
{
    return std::move(graph_);
}

AliveSupervisionConfig Config::takeAliveSupervision()
{
    return std::move(alive_supervision_);
}

}  // namespace score::mw::lifecycle::internal::configuration
