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

#include "score/mw/launch_manager/configuration/configuration_adapter.hpp"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/osal/num_cores.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
#include <map>
#include <set>

namespace score::mw::launch_manager::configuration {

const uint32_t ConfigurationAdapter::kDefaultProcessExecutionError = 1U;
uint32_t ConfigurationAdapter::kDefaultProcessorAffinityMask() {
    return static_cast<uint32_t>((1ULL << score::lcm::internal::osal::getNumCores()) - 1ULL);
}
const int32_t ConfigurationAdapter::kDefaultSchedulingPolicy = SCHED_OTHER;
const int32_t ConfigurationAdapter::kDefaultRealtimeSchedulingPriority = 99;
const int32_t ConfigurationAdapter::kDefaultNormalSchedulingPriority = 0;

score::lcm::internal::osal::CommsType ConfigurationAdapter::mapApplicationType(
    ApplicationType app_type) const {
    switch (app_type) {
        case ApplicationType::Reporting:
        case ApplicationType::ReportingAndSupervised:
            return score::lcm::internal::osal::CommsType::kReporting;
        case ApplicationType::StateManager:
            return score::lcm::internal::osal::CommsType::kControlClient;
        case ApplicationType::Native:
        default:
            return score::lcm::internal::osal::CommsType::kNoComms;
    }
}

bool ConfigurationAdapter::initialize(const Config& config) {
    LM_LOG_DEBUG() << "Loading LCM Configurations...";

    const char* env_val = getenv("ECUCFG_ENV_VAR_ROOTFOLDER");
    if (!env_val || strlen(env_val) == 0) {
        setenv("ECUCFG_ENV_VAR_ROOTFOLDER", "/opt/internal/launch_manager/etc/ecu-cfg", 1);
    }
    LM_LOG_DEBUG() << "ECUCFG_ENV_VAR_ROOTFOLDER set successfully";

    return buildFromConfig(config);
}

void ConfigurationAdapter::deinitialize() {
    for (auto& process_group : process_groups_) {
        for (auto& process : process_group.processes_) {
            for (size_t i = 0U;
                 i < score::lcm::internal::kArgvArraySize && process.startup_config_.argv_[i] != nullptr;
                 ++i) {
                free(const_cast<char*>(process.startup_config_.argv_[i]));
                process.startup_config_.argv_[i] = nullptr;
            }
            for (size_t i = 0U; process.startup_config_.envp_[i] != nullptr; ++i) {
                free(process.startup_config_.envp_[i]);
                process.startup_config_.envp_[i] = nullptr;
            }
        }
    }
}

bool ConfigurationAdapter::buildFromConfig(const Config& config) {
    const std::string initial_run_target_name = std::string(config.initialRunTarget());

    ProcessGroup pg;
    const IdentifierHash pg_name{"MainPG"};
    pg.name_ = pg_name;
    pg.sw_cluster_ = IdentifierHash{"DefaultSoftwareCluster"};
    pg.off_state_ = IdentifierHash{"MainPG/Off"};

    const auto& fallback = config.fallbackRunTarget();
    const auto& run_targets = config.runTargets();

    IdentifierHash recovery_state{"Recovery"};
    for (const auto& rt : run_targets) {
        if (rt.recovery_action.run_target == "fallback_run_target") {
            recovery_state = IdentifierHash{std::string("MainPG/") + "fallback_run_target"};
            break;
        }
    }
    pg.recovery_state_ = recovery_state;

    std::map<std::string, uint32_t> component_to_process_index;
    uint32_t process_index = 0;

    for (const auto& comp : config.components()) {
        OsProcess os_process{};
        os_process.process_id_ = IdentifierHash{comp.name};
        os_process.process_number_ = process_index;

        auto& startup = os_process.startup_config_;
        const auto& deploy = comp.deployment_config;
        const auto& props = comp.component_properties;

        std::string exec_path = deploy.bin_dir;
        if (!exec_path.empty() && exec_path.back() != '/') {
            exec_path += '/';
        }
        exec_path += props.binary_name;
        startup.executable_path_ = exec_path;
        startup.short_name_ = comp.name;

        startup.uid_ = deploy.sandbox.uid;
        startup.gid_ = deploy.sandbox.gid;
        startup.supplementary_gids_ = deploy.sandbox.supplementary_group_ids;
        startup.security_policy_ = deploy.sandbox.security_policy.value_or("");
        startup.scheduling_policy_ = deploy.sandbox.scheduling_policy;
        startup.scheduling_priority_ = deploy.sandbox.scheduling_priority;
        startup.cpu_mask_ = kDefaultProcessorAffinityMask();

        startup.resource_limits_.stack_ = 0U;
        startup.resource_limits_.cpu_ = 0U;
        startup.resource_limits_.data_ = 0U;
        startup.resource_limits_.as_ = 0U;
        if (deploy.sandbox.max_memory_usage.has_value()) {
            startup.resource_limits_.as_ = *deploy.sandbox.max_memory_usage;
        }

        startup.comms_type_ = mapApplicationType(props.application_profile.application_type);

        size_t arg_index = 0U;
        startup.argv_[arg_index] = strdup(startup.executable_path_.c_str());
        ++arg_index;
        size_t max_args = std::min(props.process_arguments.size(),
                                   static_cast<size_t>(score::lcm::internal::kMaxArg));
        for (size_t i = 0U; i < max_args; ++i) {
            startup.argv_[arg_index++] = strdup(props.process_arguments[i].c_str());
        }

        size_t env_index = 0U;
        size_t max_env = std::min(deploy.environmental_variables.size(),
                                  static_cast<size_t>(score::lcm::internal::kMaxEnv));
        size_t env_count = 0;
        for (const auto& ev : deploy.environmental_variables) {
            if (env_count >= max_env) break;
            std::string env_str = std::string(ev.key()) + "=" + std::string(ev.value());
            startup.envp_[env_index++] = strdup(env_str.c_str());
            ++env_count;
        }

        bool is_sup = (props.application_profile.application_type == ApplicationType::ReportingAndSupervised ||
                        props.application_profile.application_type == ApplicationType::StateManager);
        if (is_sup && env_index < static_cast<size_t>(score::lcm::internal::kMaxEnv)) {
            std::string iface_path = "LCM_ALIVE_INTERFACE_PATH=/lifecycle_health_" + comp.name;
            startup.envp_[env_index++] = strdup(iface_path.c_str());
        }

        startup.envp_[env_index] = nullptr;

        auto& pgm = os_process.pgm_config_;
        pgm.is_self_terminating_ = props.application_profile.is_self_terminating;
        pgm.startup_timeout_ms_ = std::chrono::milliseconds(deploy.ready_timeout_ms);
        pgm.termination_timeout_ms_ = std::chrono::milliseconds(deploy.shutdown_timeout_ms);
        pgm.execution_error_code_ = kDefaultProcessExecutionError;
        pgm.number_of_restart_attempts = 0U;
        if (deploy.ready_recovery_action.has_value()) {
            pgm.number_of_restart_attempts = deploy.ready_recovery_action->number_of_attempts;
        }

        for (const auto& dep_name : props.depends_on) {
            Dependency dep{};
            if (props.ready_condition.has_value()) {
                dep.process_state_ = (props.ready_condition->process_state == ProcessState::Running)
                    ? score::lcm::ProcessState::kRunning
                    : score::lcm::ProcessState::kTerminated;
            } else {
                dep.process_state_ = score::lcm::ProcessState::kRunning;
            }
            dep.target_process_id_ = IdentifierHash{dep_name};
            os_process.dependencies_.push_back(dep);
        }

        component_to_process_index[comp.name] = process_index;
        pg.processes_.push_back(std::move(os_process));
        ++process_index;
    }

    {
        ProcessGroupState off_state;
        off_state.name_ = IdentifierHash{"MainPG/Off"};
        pg.states_.push_back(off_state);
    }

    std::map<std::string, const RunTargetConfig*> run_target_by_name;
    for (const auto& rt : run_targets) {
        run_target_by_name[rt.name] = &rt;
    }

    std::map<std::string, const ComponentConfig*> component_by_name;
    for (const auto& comp : config.components()) {
        component_by_name[comp.name] = &comp;
    }

    std::function<void(const std::string&, std::vector<uint32_t>&, std::set<std::string>&)> resolve_depends;
    resolve_depends = [&](const std::string& dep_name,
                          std::vector<uint32_t>& indexes,
                          std::set<std::string>& visited) {
        if (!visited.insert(dep_name).second) return;
        auto comp_it = component_to_process_index.find(dep_name);
        if (comp_it != component_to_process_index.end()) {
            if (std::find(indexes.begin(), indexes.end(), comp_it->second) == indexes.end()) {
                indexes.push_back(comp_it->second);
            }
            auto comp_cfg = component_by_name.find(dep_name);
            if (comp_cfg != component_by_name.end()) {
                for (const auto& sub_dep : comp_cfg->second->component_properties.depends_on) {
                    resolve_depends(sub_dep, indexes, visited);
                }
            }
        }
        auto rt_it = run_target_by_name.find(dep_name);
        if (rt_it != run_target_by_name.end()) {
            for (const auto& sub_dep : rt_it->second->depends_on) {
                resolve_depends(sub_dep, indexes, visited);
            }
        }
    };

    for (const auto& rt : run_targets) {
        ProcessGroupState pgs;
        pgs.name_ = IdentifierHash{std::string("MainPG/") + rt.name};

        std::set<std::string> visited;
        for (const auto& dep_name : rt.depends_on) {
            resolve_depends(dep_name, pgs.process_indexes_, visited);
        }
        pg.states_.push_back(std::move(pgs));
    }

    {
        ProcessGroupState fallback_state;
        fallback_state.name_ = IdentifierHash{"MainPG/fallback_run_target"};
        for (const auto& comp_name : fallback.depends_on) {
            auto it = component_to_process_index.find(comp_name);
            if (it != component_to_process_index.end()) {
                fallback_state.process_indexes_.push_back(it->second);
            }
        }
        pg.states_.push_back(std::move(fallback_state));
    }

    for (auto& proc : pg.processes_) {
        for (auto& dep : proc.dependencies_) {
            for (uint32_t idx = 0; idx < pg.processes_.size(); ++idx) {
                if (pg.processes_[idx].process_id_ == dep.target_process_id_) {
                    dep.os_process_index_ = idx;
                    break;
                }
            }
        }
    }

    process_groups_.push_back(std::move(pg));
    process_group_names_.push_back(pg_name);

    main_pg_startup_state_ = score::lcm::internal::ProcessGroupStateID{
        pg_name,
        IdentifierHash{std::string("MainPG/") + initial_run_target_name}
    };

    LM_LOG_DEBUG() << "ConfigurationAdapter: Built configuration with "
                   << process_groups_[0].processes_.size() << " processes and "
                   << process_groups_[0].states_.size() << " states";

    return true;
}

std::optional<const std::vector<IdentifierHash>*> ConfigurationAdapter::getListOfProcessGroups() const {
    if (!process_group_names_.empty()) {
        return &process_group_names_;
    }
    return std::nullopt;
}

std::optional<IdentifierHash> ConfigurationAdapter::getSoftwareCluster(const IdentifierHash& process_group_id) const {
    auto pg = getProcessGroupByID(process_group_id);
    if (pg) {
        return pg->sw_cluster_;
    }
    return std::nullopt;
}

std::optional<uint32_t> ConfigurationAdapter::getNumberOfOsProcesses(const IdentifierHash& pg_name) const {
    auto pg = getProcessGroupByID(pg_name);
    if (pg) {
        return static_cast<uint32_t>(pg->processes_.size());
    }
    return std::nullopt;
}

IdentifierHash ConfigurationAdapter::getNameOfOffState(const IdentifierHash& pg_name) const {
    auto pg = getProcessGroupByID(pg_name);
    if (pg) {
        return pg->off_state_;
    }
    return IdentifierHash{"Off"};
}

IdentifierHash ConfigurationAdapter::getNameOfRecoveryState(const IdentifierHash& pg_name) const {
    auto pg = getProcessGroupByID(pg_name);
    if (pg) {
        return pg->recovery_state_;
    }
    return IdentifierHash{"Recovery"};
}

std::optional<const score::lcm::internal::ProcessGroupStateID*> ConfigurationAdapter::getMainPGStartupState() const {
    auto pg = getProcessGroupByID(main_pg_startup_state_.pg_name_);
    if (pg) {
        for (const auto& state : pg->states_) {
            if (state.name_ == main_pg_startup_state_.pg_state_name_) {
                return &main_pg_startup_state_;
            }
        }
        LM_LOG_DEBUG() << "Process group state not found:" << main_pg_startup_state_.pg_state_name_;
    } else {
        LM_LOG_DEBUG() << "Process group not found:" << main_pg_startup_state_.pg_name_;
    }
    return std::nullopt;
}

std::optional<const std::vector<uint32_t>*> ConfigurationAdapter::getProcessIndexesList(
    const score::lcm::internal::ProcessGroupStateID& pg_state_id) const {
    auto state = getProcessGroupStateByID(pg_state_id);
    if (state) {
        return &state->process_indexes_;
    }
    LM_LOG_DEBUG() << "Process group state '" << pg_state_id.pg_state_name_
                   << "' not found in group '" << pg_state_id.pg_name_ << "'.";
    return std::nullopt;
}

std::optional<const OsProcess*> ConfigurationAdapter::getOsProcessConfiguration(
    const IdentifierHash& pg_name, const uint32_t index) const {
    if (auto pg = getProcessGroupByNameAndIndex(pg_name, index)) {
        return &(*pg)->processes_[index];
    }
    LM_LOG_DEBUG() << "Unable to retrieve process configuration for process group" << pg_name
                   << "with index" << index;
    return std::nullopt;
}

std::optional<const DependencyList*> ConfigurationAdapter::getOsProcessDependencies(
    const IdentifierHash& pg_name, const uint32_t index) const {
    if (auto pg = getProcessGroupByNameAndIndex(pg_name, index)) {
        return &(*pg)->processes_[index].dependencies_;
    }
    LM_LOG_DEBUG() << "Unable to retrieve process dependencies for process group" << pg_name
                   << "with index" << index;
    return std::nullopt;
}

ProcessGroup* ConfigurationAdapter::getProcessGroupByID(const IdentifierHash& pg_name) const {
    for (const auto& pg : process_groups_) {
        if (pg.name_ == pg_name) {
            return const_cast<ProcessGroup*>(&pg);
        }
    }
    return nullptr;
}

ProcessGroupState* ConfigurationAdapter::getProcessGroupStateByID(const score::lcm::internal::ProcessGroupStateID& pg_id) const {
    ProcessGroup* pg = getProcessGroupByID(pg_id.pg_name_);
    if (pg) {
        for (auto& state : pg->states_) {
            if (state.name_ == pg_id.pg_state_name_) {
                return &state;
            }
        }
    }
    return nullptr;
}

ProcessGroupState* ConfigurationAdapter::getProcessGroupStateByID(
    ProcessGroup& pg, const IdentifierHash& state_name) const {
    for (auto& state : pg.states_) {
        if (state.name_ == state_name) {
            return &state;
        }
    }
    return nullptr;
}

std::optional<const ProcessGroup*> ConfigurationAdapter::getProcessGroupByNameAndIndex(
    const IdentifierHash& pg_name, const uint32_t index) const {
    auto pg = getProcessGroupByID(pg_name);
    if (pg) {
        if (index < pg->processes_.size()) {
            return pg;
        }
        LM_LOG_DEBUG() << "Process index" << index << "is out of bounds in process group" << pg_name;
    } else {
        LM_LOG_DEBUG() << "Process group not found:" << pg_name;
    }
    return std::nullopt;
}

}  // namespace score::mw::launch_manager::configuration
