// *******************************************************************************
// Copyright (c) 2026 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

#include "config.hpp"

#include <utility>

namespace score::launch_manager::config {

Config::Config(std::vector<ComponentConfig> components,
               std::vector<RunTargetConfig> run_targets,
               std::string initial_run_target,
               std::optional<FallbackRunTargetConfig> fallback_run_target,
               std::optional<AliveSupervisionConfig> alive_supervision,
               std::optional<WatchdogConfig> watchdog)
    : components_{std::move(components)},
      run_targets_{std::move(run_targets)},
      initial_run_target_{std::move(initial_run_target)},
      fallback_run_target_{std::move(fallback_run_target)},
      alive_supervision_{std::move(alive_supervision)},
      watchdog_{std::move(watchdog)}
{
}

Config::Builder& Config::Builder::setComponents(std::vector<ComponentConfig> components)
{
    components_ = std::move(components);
    return *this;
}

Config::Builder& Config::Builder::setRunTargets(std::vector<RunTargetConfig> run_targets)
{
    run_targets_ = std::move(run_targets);
    return *this;
}

Config::Builder& Config::Builder::setInitialRunTarget(std::string initial_run_target)
{
    initial_run_target_ = std::move(initial_run_target);
    return *this;
}

Config::Builder& Config::Builder::setFallbackRunTarget(FallbackRunTargetConfig fallback)
{
    fallback_run_target_ = std::move(fallback);
    return *this;
}

Config::Builder& Config::Builder::setAliveSupervision(AliveSupervisionConfig alive_supervision)
{
    alive_supervision_ = std::move(alive_supervision);
    return *this;
}

Config::Builder& Config::Builder::setWatchdog(WatchdogConfig watchdog)
{
    watchdog_ = std::move(watchdog);
    return *this;
}

Config Config::Builder::build()
{
    return Config(std::move(components_),
                  std::move(run_targets_),
                  std::move(initial_run_target_),
                  std::move(fallback_run_target_),
                  std::move(alive_supervision_),
                  std::move(watchdog_));
}

}  // namespace score::launch_manager::config
