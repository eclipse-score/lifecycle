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

#include "score/filesystem/path.h"
#include "score/os/errno.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <cstdint>
#include <limits>
#include <sched.h>
#include <sys/types.h>
#include <vector>

namespace score::launch_manager::config
{
namespace
{

namespace fb = score::launch_manager::config::fb;

using ::testing::Eq;
using ::testing::IsFalse;
using ::testing::IsNull;
using ::testing::IsTrue;
using ::testing::StrEq;

const score::filesystem::Path kTestPath{"/tmp/test_config.bin"};

std::vector<uint8_t> finishBuffer(::flatbuffers::FlatBufferBuilder& fbb,
                                  ::flatbuffers::Offset<fb::LaunchManagerConfig> root)
{
    fb::FinishLaunchManagerConfigBuffer(fbb, root);
    const auto* buf = fbb.GetBufferPointer();
    return {buf, buf + fbb.GetSize()};
}

// ============================================================================
// Helper functions to reduce test boilerplate
// ============================================================================

::flatbuffers::Offset<fb::Sandbox> buildDefaultSandbox(::flatbuffers::FlatBufferBuilder& fbb)
{
    return fb::CreateSandbox(fbb,
                             0 /*uid*/,
                             0 /*gid*/,
                             0 /*supplementary_group_ids*/,
                             0 /*security_policy*/,
                             fb::SchedulingPolicy::OTHER,
                             0 /*scheduling_priority*/);
}

::flatbuffers::Offset<fb::ComponentProperties> buildDefaultComponentProperties(::flatbuffers::FlatBufferBuilder& fbb)
{
    auto bin_name = fbb.CreateString("default_bin");
    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false);
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    return fb::CreateComponentProperties(
        fbb, bin_name, app_profile, 0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);
}

::flatbuffers::Offset<fb::DeploymentConfig> buildDefaultDeploymentConfig(::flatbuffers::FlatBufferBuilder& fbb)
{
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    return fb::CreateDeploymentConfig(
        fbb, 1.0 /*ready_timeout*/, 1.0 /*shutdown_timeout*/, 0 /*environmental_variables*/,
        bin_dir, work_dir, 0 /*ready_recovery_action*/, 0 /*recovery_action*/, sandbox);
}

::flatbuffers::Offset<fb::Component> buildDefaultComponent(
    ::flatbuffers::FlatBufferBuilder& fbb,
    const char* name,
    ::flatbuffers::Offset<fb::ComponentProperties> comp_props = 0,
    ::flatbuffers::Offset<fb::DeploymentConfig> deploy = 0)
{
    if (comp_props.IsNull())
    {
        comp_props = buildDefaultComponentProperties(fbb);
    }
    if (deploy.IsNull())
    {
        deploy = buildDefaultDeploymentConfig(fbb);
    }
    auto comp_name = fbb.CreateString(name);
    return fb::CreateComponent(fbb, comp_name, 0 /*description*/, comp_props, deploy);
}

std::vector<uint8_t> buildConfigWithComponents(
    ::flatbuffers::FlatBufferBuilder& fbb,
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<fb::Component>>> comps)
{
    auto fallback = fb::CreateFallbackRunTarget(fbb, 0 /*description*/, 0 /*depends_on*/, 1.0 /*transition_timeout*/);
    auto alive_sup = fb::CreateAliveSupervision(fbb, 1.0 /*evaluation_cycle*/);
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb, 1 /*schema_version*/, comps, rts, irt, fallback, alive_sup);
    return finishBuffer(fbb, config);
}

std::vector<uint8_t> buildConfigWithRunTargets(
    ::flatbuffers::FlatBufferBuilder& fbb,
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<fb::RunTarget>>> rts)
{
    auto fallback = fb::CreateFallbackRunTarget(fbb, 0 /*description*/, 0 /*depends_on*/, 1.0 /*transition_timeout*/);
    auto alive_sup = fb::CreateAliveSupervision(fbb, 1.0 /*evaluation_cycle*/);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb, 1 /*schema_version*/, comps, rts, irt, fallback, alive_sup);
    return finishBuffer(fbb, config);
}

// ============================================================================

struct MockBufferLoader
{
    static score::os::Result<std::vector<uint8_t>> load(const score::filesystem::Path&)
    {
        return result_;
    }
    static score::os::Result<std::vector<uint8_t>> result_;
};

score::os::Result<std::vector<uint8_t>> MockBufferLoader::result_{std::vector<uint8_t>{}};

class FlatbufferConfigLoaderTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
        MockBufferLoader::result_ = std::vector<uint8_t>{};
    }

    std::vector<uint8_t> buildMinimalConfig(int32_t schema_version = 1, const char* initial_run_target = "Startup")
    {
        ::flatbuffers::FlatBufferBuilder fbb;
        auto irt = fbb.CreateString(initial_run_target);
        auto fallback =
            fb::CreateFallbackRunTarget(fbb, 0 /*description*/, 0 /*depends_on*/, 1.0 /*transition_timeout*/);
        auto alive_sup = fb::CreateAliveSupervision(fbb, 1.0 /*evaluation_cycle*/);
        auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
        auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
        auto config =
            fb::CreateLaunchManagerConfig(fbb, schema_version, comps, rts, irt, fallback, alive_sup);
        return finishBuffer(fbb, config);
    }

    score::cpp::expected<Config, IConfigLoader::Error> loadBuffer(const std::vector<uint8_t>& buffer)
    {
        MockBufferLoader::result_ = buffer;
        return loader_.load(kTestPath);
    }

    FlatbufferConfigLoaderImpl<MockBufferLoader> loader_;
};

// ============================================================================
// Happy path tests
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, LoadMinimalConfig)
{
    RecordProperty("Description", "Loads a minimal config with only required fields.");

    auto result = loadBuffer(buildMinimalConfig(1, "Startup"));

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->initialRunTarget(), Eq("Startup"));
    EXPECT_THAT(result->components().empty(), IsTrue());
    EXPECT_THAT(result->runTargets().empty(), IsTrue());
    EXPECT_THAT(result->watchdog().has_value(), IsFalse());
}

TEST_F(FlatbufferConfigLoaderTest, LoadSingleComponent)
{
    RecordProperty("Description", "Loads a config with one fully-populated component.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto alive_sup = fb::CreateComponentAliveSupervision(
        fbb, 0.5 /*reporting_cycle*/, 2 /*failed_cycles_tolerance*/, 1 /*min_indications*/, 3 /*max_indications*/);
    auto app_profile = fb::CreateApplicationProfile(
        fbb, fb::ApplicationType::Reporting_And_Supervised, true /*is_self_terminating*/, alive_sup);
    auto bin_name = fbb.CreateString("my_binary");
    auto dep = fbb.CreateString("other_comp");
    auto deps_vec = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{dep});
    auto arg = fbb.CreateString("--verbose");
    auto args_vec = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{arg});
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile, deps_vec, args_vec, ready_cond);

    auto bin_dir = fbb.CreateString("/opt/bin");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(
        fbb, 1.5 /*ready_timeout*/, 2.5 /*shutdown_timeout*/, 0 /*environmental_variables*/,
        bin_dir, work_dir, 0 /*ready_recovery_action*/, 0 /*recovery_action*/, sandbox);

    auto comp_name = fbb.CreateString("TestComponent");
    auto comp_desc = fbb.CreateString("A test component");
    auto component = fb::CreateComponent(fbb, comp_name, comp_desc, comp_props, deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{component});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components().size(), Eq(1U));

    const auto& comp = result->components()[0];
    EXPECT_THAT(comp.name, Eq("TestComponent"));
    EXPECT_THAT(comp.description, Eq("A test component"));
    EXPECT_THAT(comp.component_properties.binary_name, Eq("my_binary"));
    EXPECT_THAT(comp.component_properties.application_profile.application_type,
                Eq(ApplicationType::ReportingAndSupervised));
    EXPECT_THAT(comp.component_properties.application_profile.is_self_terminating, IsTrue());
    ASSERT_THAT(comp.component_properties.application_profile.alive_supervision.has_value(), IsTrue());
    EXPECT_THAT(comp.component_properties.application_profile.alive_supervision->reporting_cycle_ms, Eq(500U));
    EXPECT_THAT(comp.component_properties.application_profile.alive_supervision->failed_cycles_tolerance, Eq(2U));
    ASSERT_THAT(comp.component_properties.application_profile.alive_supervision->min_indications.has_value(), IsTrue());
    EXPECT_THAT(*comp.component_properties.application_profile.alive_supervision->min_indications, Eq(1U));
    ASSERT_THAT(comp.component_properties.application_profile.alive_supervision->max_indications.has_value(), IsTrue());
    EXPECT_THAT(*comp.component_properties.application_profile.alive_supervision->max_indications, Eq(3U));
    ASSERT_THAT(comp.component_properties.depends_on.size(), Eq(1U));
    EXPECT_THAT(comp.component_properties.depends_on[0], Eq("other_comp"));
    ASSERT_THAT(comp.component_properties.process_arguments.size(), Eq(1U));
    EXPECT_THAT(comp.component_properties.process_arguments[0], Eq("--verbose"));
    ASSERT_THAT(comp.component_properties.ready_condition.has_value(), IsTrue());
    EXPECT_THAT(comp.component_properties.ready_condition->process_state, Eq(ProcessState::Running));
    EXPECT_THAT(comp.deployment_config.ready_timeout_ms, Eq(1500U));
    EXPECT_THAT(comp.deployment_config.shutdown_timeout_ms, Eq(2500U));
    EXPECT_THAT(comp.deployment_config.bin_dir, Eq("/opt/bin"));
    EXPECT_THAT(comp.deployment_config.working_dir, Eq("/tmp"));
}

TEST_F(FlatbufferConfigLoaderTest, LoadRunTargets)
{
    RecordProperty("Description", "Loads run targets with dependencies and transition timeout.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto switch_target = fbb.CreateString("SafeState");
    auto switch_action = fb::CreateSwitchRunTargetAction(fbb, switch_target);
    auto rt_name = fbb.CreateString("Startup");
    auto rt_desc = fbb.CreateString("Initial state");
    auto rt_dep = fbb.CreateString("component_a");
    auto rt_deps = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{rt_dep});
    auto rt = fb::CreateRunTarget(fbb, rt_name, rt_desc, rt_deps, 5.0 /*transition_timeout*/, switch_action);
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{rt});

    auto result = loadBuffer(buildConfigWithRunTargets(fbb, rts));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->runTargets().size(), Eq(1U));

    const auto& target = result->runTargets()[0];
    EXPECT_THAT(target.name, Eq("Startup"));
    EXPECT_THAT(target.description, Eq("Initial state"));
    ASSERT_THAT(target.depends_on.size(), Eq(1U));
    EXPECT_THAT(target.depends_on[0], Eq("component_a"));
    EXPECT_THAT(target.transition_timeout_ms, Eq(5000U));
    EXPECT_THAT(target.recovery_action.run_target, Eq("SafeState"));
}

TEST_F(FlatbufferConfigLoaderTest, LoadFallbackRunTarget)
{
    RecordProperty("Description", "Loads fallback run target with all fields.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto fb_desc = fbb.CreateString("Fallback state");
    auto fb_dep = fbb.CreateString("critical_comp");
    auto fb_deps = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{fb_dep});
    auto fallback = fb::CreateFallbackRunTarget(fbb, fb_desc, fb_deps, 10.0 /*transition_timeout*/);

    auto alive_sup = fb::CreateAliveSupervision(fbb, 1.0 /*evaluation_cycle*/);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb, 1 /*schema_version*/, comps, rts, irt, fallback, alive_sup);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsTrue());
    const auto& fb = result->fallbackRunTarget();
    EXPECT_THAT(fb.description, Eq("Fallback state"));
    ASSERT_THAT(fb.depends_on.size(), Eq(1U));
    EXPECT_THAT(fb.depends_on[0], Eq("critical_comp"));
    EXPECT_THAT(fb.transition_timeout_ms, Eq(10000U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadAliveSupervision)
{
    RecordProperty("Description", "Loads global alive supervision config.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto alive_sup = fb::CreateAliveSupervision(fbb, 0.25 /*evaluation_cycle*/);
    auto fallback = fb::CreateFallbackRunTarget(fbb, 0 /*description*/, 0 /*depends_on*/, 1.0 /*transition_timeout*/);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb, 1 /*schema_version*/, comps, rts, irt, fallback, alive_sup);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->aliveSupervision().evaluation_cycle_ms, Eq(250U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadWatchdog)
{
    RecordProperty("Description", "Loads watchdog config with all fields.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto dev_path = fbb.CreateString("/dev/watchdog0");
    auto watchdog = fb::CreateWatchdog(
        fbb, dev_path, 30.0 /*max_timeout*/, true /*deactivate_on_shutdown*/, false /*require_magic_close*/);

    auto fallback = fb::CreateFallbackRunTarget(fbb, 0 /*description*/, 0 /*depends_on*/, 1.0 /*transition_timeout*/);
    auto alive_sup = fb::CreateAliveSupervision(fbb, 1.0 /*evaluation_cycle*/);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb, 1 /*schema_version*/, comps, rts, irt, fallback, alive_sup, watchdog);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->watchdog().has_value(), IsTrue());

    const auto& wd = result->watchdog().value();
    EXPECT_THAT(wd.device_file_path, Eq("/dev/watchdog0"));
    EXPECT_THAT(wd.max_timeout_ms, Eq(30000U));
    EXPECT_THAT(wd.deactivate_on_shutdown, IsTrue());
    EXPECT_THAT(wd.require_magic_close, IsFalse());
}

TEST_F(FlatbufferConfigLoaderTest, LoadRestartRecoveryAction)
{
    RecordProperty("Description", "Recovery action variant holds RestartAction with correct fields on a component.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto restart = fb::CreateRestartAction(fbb, 3 /*number_of_attempts*/, 1.5 /*delay_before_restart*/);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(fbb,
                                             1.0 /*ready_timeout*/,
                                             1.0 /*shutdown_timeout*/,
                                             0 /*environmental_variables*/,
                                             bin_dir,
                                             work_dir,
                                             restart,
                                             0 /*recovery_action*/,
                                             sandbox);

    auto comp = buildDefaultComponent(fbb, "restart_comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components().size(), Eq(1U));
    ASSERT_THAT(result->components()[0].deployment_config.ready_recovery_action.has_value(), IsTrue());

    const auto& ra = result->components()[0].deployment_config.ready_recovery_action.value();
    EXPECT_THAT(ra.number_of_attempts, Eq(3U));
    EXPECT_THAT(ra.delay_before_restart_ms, Eq(1500U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadSwitchRunTargetAction)
{
    RecordProperty("Description", "Recovery action variant holds SwitchRunTargetAction.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto target_name = fbb.CreateString("Fallback");
    auto switch_action = fb::CreateSwitchRunTargetAction(fbb, target_name);
    auto rt_name = fbb.CreateString("Startup");
    auto rt = fb::CreateRunTarget(
        fbb, rt_name, 0 /*description*/, 0 /*depends_on*/, 1.0 /*transition_timeout*/, switch_action);
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{rt});

    auto result = loadBuffer(buildConfigWithRunTargets(fbb, rts));

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->runTargets()[0].recovery_action.run_target, Eq("Fallback"));
}

TEST_F(FlatbufferConfigLoaderTest, LoadSandbox)
{
    RecordProperty("Description", "Loads sandbox config including optional fields.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto sec_policy = fbb.CreateString("strict");
    auto supp_gids = fbb.CreateVector(std::vector<uint32_t>{100, 200});
    auto sandbox = fb::CreateSandbox(fbb,
                                     1000 /*uid*/,
                                     1000 /*gid*/,
                                     supp_gids,
                                     sec_policy,
                                     fb::SchedulingPolicy::FIFO,
                                     50 /*scheduling_priority*/,
                                     4096 /*max_memory_usage*/,
                                     80 /*max_cpu_usage*/);

    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb,
                                             0.5 /*ready_timeout*/,
                                             0.5 /*shutdown_timeout*/,
                                             0 /*environmental_variables*/,
                                             bin_dir,
                                             work_dir,
                                             0 /*ready_recovery_action*/,
                                             0 /*recovery_action*/,
                                             sandbox);

    auto comp = buildDefaultComponent(fbb, "sandboxed_comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components().size(), Eq(1U));

    const auto& sb = result->components()[0].deployment_config.sandbox;
    EXPECT_THAT(sb.uid, Eq(1000U));
    EXPECT_THAT(sb.gid, Eq(1000U));
    EXPECT_THAT(sb.supplementary_group_ids, Eq(std::vector<gid_t>{100, 200}));
    ASSERT_THAT(sb.security_policy.has_value(), IsTrue());
    EXPECT_THAT(sb.security_policy.value(), Eq("strict"));
    EXPECT_THAT(sb.scheduling_policy, Eq(SCHED_FIFO));
    EXPECT_THAT(sb.scheduling_priority, Eq(50));
    ASSERT_THAT(sb.max_memory_usage.has_value(), IsTrue());
    EXPECT_THAT(sb.max_memory_usage.value(), Eq(4096U));
    ASSERT_THAT(sb.max_cpu_usage.has_value(), IsTrue());
    EXPECT_THAT(sb.max_cpu_usage.value(), Eq(80U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadComponentAliveSupervision)
{
    RecordProperty("Description", "Per-component alive supervision is mapped correctly.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto comp_alive_sup = fb::CreateComponentAliveSupervision(
        fbb, 1.0 /*reporting_cycle*/, 3 /*failed_cycles_tolerance*/, 2 /*min_indications*/, 5 /*max_indications*/);
    auto app_profile = fb::CreateApplicationProfile(
        fbb, fb::ApplicationType::Reporting_And_Supervised, false /*is_self_terminating*/, comp_alive_sup);
    auto bin_name = fbb.CreateString("supervised_bin");
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(
        fbb, bin_name, app_profile, 0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);

    auto comp = buildDefaultComponent(fbb, "supervised_comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    const auto& as = result->components()[0].component_properties.application_profile.alive_supervision;
    ASSERT_THAT(as.has_value(), IsTrue());
    EXPECT_THAT(as->reporting_cycle_ms, Eq(1000U));
    EXPECT_THAT(as->failed_cycles_tolerance, Eq(3U));
    ASSERT_THAT(as->min_indications.has_value(), IsTrue());
    EXPECT_THAT(*as->min_indications, Eq(2U));
    ASSERT_THAT(as->max_indications.has_value(), IsTrue());
    EXPECT_THAT(*as->max_indications, Eq(5U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadEnvironmentalVariables)
{
    RecordProperty("Description", "Environmental variables are stored as key=value strings.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto k1 = fbb.CreateString("PATH");
    auto v1 = fbb.CreateString("/usr/bin");
    auto ev1 = fb::CreateEnvironmentalVariable(fbb, k1, v1);
    auto k2 = fbb.CreateString("HOME");
    auto v2 = fbb.CreateString("/root");
    auto ev2 = fb::CreateEnvironmentalVariable(fbb, k2, v2);
    auto env_vars = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::EnvironmentalVariable>>{ev1, ev2});

    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(
        fbb, 0.5 /*ready_timeout*/, 0.5 /*shutdown_timeout*/, env_vars,
        bin_dir, work_dir, 0 /*ready_recovery_action*/, 0 /*recovery_action*/, sandbox);

    auto comp = buildDefaultComponent(fbb, "env_comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    const auto& env = result->components()[0].deployment_config.environmental_variables;
    ASSERT_THAT(env.size(), Eq(2U));
    auto it = env.begin();
    EXPECT_THAT(it->key(), Eq("PATH"));
    EXPECT_THAT(it->value(), Eq("/usr/bin"));
    ++it;
    EXPECT_THAT(it->key(), Eq("HOME"));
    EXPECT_THAT(it->value(), Eq("/root"));
    ASSERT_THAT(env.size(), Eq(2U));
    EXPECT_THAT(env.envp()[0], StrEq("PATH=/usr/bin"));
    EXPECT_THAT(env.envp()[1], StrEq("HOME=/root"));
    EXPECT_THAT(env.envp()[2], IsNull());
}

TEST_F(FlatbufferConfigLoaderTest, LoadMultipleComponents)
{
    RecordProperty("Description", "Multiple components are all present and in order.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto comp_a = buildDefaultComponent(fbb, "CompA");
    auto comp_b = buildDefaultComponent(fbb, "CompB");
    auto comp_c = buildDefaultComponent(fbb, "CompC");
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp_a, comp_b, comp_c});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components().size(), Eq(3U));
    EXPECT_THAT(result->components()[0].name, Eq("CompA"));
    EXPECT_THAT(result->components()[1].name, Eq("CompB"));
    EXPECT_THAT(result->components()[2].name, Eq("CompC"));
}

// ============================================================================
// Optional / required field tests
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, OptionalWatchdogAbsent)
{
    RecordProperty("Description", "When no watchdog is present, it is nullopt.");

    auto result = loadBuffer(buildMinimalConfig());

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->watchdog().has_value(), IsFalse());
}

TEST_F(FlatbufferConfigLoaderTest, OptionalReadyConditionAbsent)
{
    RecordProperty("Description", "When no ready_condition is present on a component, it is nullopt.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto bin_name = fbb.CreateString("no_rc_bin");
    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile);

    auto comp = buildDefaultComponent(fbb, "no_rc_comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->components()[0].component_properties.ready_condition.has_value(), IsFalse());
}

TEST_F(FlatbufferConfigLoaderTest, OptionalAliveSupervisionIndicationsAbsent)
{
    RecordProperty("Description", "When min/max_indications are absent, they are nullopt.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto comp_alive_sup = fb::CreateComponentAliveSupervision(fbb, 1.0 /*reporting_cycle*/, 3 /*failed_cycles_tolerance*/);
    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false, comp_alive_sup);
    auto bin_name = fbb.CreateString("no_ind_bin");
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile, 0, 0, ready_cond);

    auto comp = buildDefaultComponent(fbb, "no_ind_comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    const auto& as = result->components()[0].component_properties.application_profile.alive_supervision;
    ASSERT_THAT(as.has_value(), IsTrue());
    EXPECT_THAT(as->min_indications.has_value(), IsFalse());
    EXPECT_THAT(as->max_indications.has_value(), IsFalse());
}

// ============================================================================
// Enum mapping tests
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, MapNativeApplicationType)
{
    RecordProperty("Description", "ApplicationType_Native maps to ApplicationType::Native.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false);
    auto bin_name = fbb.CreateString("native_bin");
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(
        fbb, bin_name, app_profile, 0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);

    auto comp = buildDefaultComponent(fbb, "native_comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->components()[0].component_properties.application_profile.application_type,
                Eq(ApplicationType::Native));
}

TEST_F(FlatbufferConfigLoaderTest, MapReportingAndSupervisedType)
{
    RecordProperty("Description",
                   "ApplicationType_Reporting_And_Supervised maps to ApplicationType::ReportingAndSupervised.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Reporting_And_Supervised, false);
    auto bin_name = fbb.CreateString("supervised_bin");
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(
        fbb, bin_name, app_profile, 0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);

    auto comp = buildDefaultComponent(fbb, "supervised_comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->components()[0].component_properties.application_profile.application_type,
                Eq(ApplicationType::ReportingAndSupervised));
}

TEST_F(FlatbufferConfigLoaderTest, MapTerminatedProcessState)
{
    RecordProperty("Description", "ProcessState_Terminated maps to ProcessState::Terminated.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false);
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Terminated);
    auto bin_name = fbb.CreateString("term_bin");
    auto comp_props = fb::CreateComponentProperties(
        fbb, bin_name, app_profile, 0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);

    auto comp = buildDefaultComponent(fbb, "term_comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components()[0].component_properties.ready_condition.has_value(), IsTrue());
    EXPECT_THAT(result->components()[0].component_properties.ready_condition->process_state,
                Eq(ProcessState::Terminated));
}

// ============================================================================
// Error path tests
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, LoadBufferFailsWithNoSuchFileReturnsFileNotFound)
{
    RecordProperty("Description", "When LoadBuffer fails with ENOENT, returns FileNotFound error.");

    MockBufferLoader::result_ = score::cpp::make_unexpected(score::os::Error::createFromErrno(ENOENT));

    auto result = loader_.load(kTestPath);

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::FileNotFound));
}

TEST_F(FlatbufferConfigLoaderTest, LoadBufferFailsWithPermissionDeniedReturnsInsufficientPermission)
{
    RecordProperty("Description", "When LoadBuffer fails with EACCES, returns InsufficientPermission error.");

    MockBufferLoader::result_ = score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES));

    auto result = loader_.load(kTestPath);

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InsufficientPermission));
}

TEST_F(FlatbufferConfigLoaderTest, LoadBufferFailsWithGenericErrorReturnsGeneralError)
{
    RecordProperty("Description", "When LoadBuffer fails with a generic OS error, returns GeneralError.");

    MockBufferLoader::result_ = score::cpp::make_unexpected(score::os::Error::createFromErrno(EIO));

    auto result = loader_.load(kTestPath);

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::GeneralError));
}

TEST_F(FlatbufferConfigLoaderTest, CorruptedBufferReturnsInvalidFormat)
{
    RecordProperty("Description", "Corrupted binary data returns InvalidFormat error.");

    auto result = loadBuffer({0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01, 0x02, 0x03});

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, WrongSchemaVersionReturnsUnsupportedVersion)
{
    RecordProperty("Description", "A config with an unexpected schema_version returns UnsupportedVersion.");

    auto result = loadBuffer(buildMinimalConfig(99, "Startup"));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::UnsupportedVersion));
}

TEST_F(FlatbufferConfigLoaderTest, SandboxUidOutOfRangeReturnsInvalidFormat)
{
    RecordProperty("Description", "A sandbox uid exceeding uid_t range returns InvalidFormat.");

    if constexpr (std::numeric_limits<uid_t>::max() >= std::numeric_limits<uint32_t>::max())
    {
        GTEST_SKIP() << "uid_t range covers all uint32_t values on this platform";
    }

    ::flatbuffers::FlatBufferBuilder fbb;

    auto sandbox = fb::CreateSandbox(fbb,
                                     std::numeric_limits<uint32_t>::max() /*uid*/,
                                     1000 /*gid*/,
                                     0 /*supplementary_group_ids*/,
                                     0 /*security_policy*/,
                                     fb::SchedulingPolicy::OTHER,
                                     0 /*scheduling_priority*/);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb, 1.0, 1.0, 0, bin_dir, work_dir, 0, 0, sandbox);

    auto comp = buildDefaultComponent(fbb, "bad_uid_comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, SandboxGidOutOfRangeReturnsInvalidFormat)
{
    RecordProperty("Description", "A sandbox gid exceeding gid_t range returns InvalidFormat.");

    if constexpr (std::numeric_limits<gid_t>::max() >= std::numeric_limits<uint32_t>::max())
    {
        GTEST_SKIP() << "gid_t range covers all uint32_t values on this platform";
    }

    ::flatbuffers::FlatBufferBuilder fbb;

    auto sandbox = fb::CreateSandbox(fbb,
                                     1000 /*uid*/,
                                     std::numeric_limits<uint32_t>::max() /*gid*/,
                                     0 /*supplementary_group_ids*/,
                                     0 /*security_policy*/,
                                     fb::SchedulingPolicy::OTHER,
                                     0 /*scheduling_priority*/);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb, 1.0, 1.0, 0, bin_dir, work_dir, 0, 0, sandbox);

    auto comp = buildDefaultComponent(fbb, "bad_gid_comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, SandboxSupplementaryGidOutOfRangeReturnsInvalidFormat)
{
    RecordProperty("Description", "A supplementary group id exceeding gid_t range returns InvalidFormat.");

    if constexpr (std::numeric_limits<gid_t>::max() >= std::numeric_limits<uint32_t>::max())
    {
        GTEST_SKIP() << "gid_t range covers all uint32_t values on this platform";
    }

    ::flatbuffers::FlatBufferBuilder fbb;

    auto supp_gids = fbb.CreateVector(std::vector<uint32_t>{100, std::numeric_limits<uint32_t>::max()});
    auto sandbox = fb::CreateSandbox(fbb,
                                     1000 /*uid*/,
                                     1000 /*gid*/,
                                     supp_gids,
                                     0 /*security_policy*/,
                                     fb::SchedulingPolicy::OTHER,
                                     0 /*scheduling_priority*/);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb, 1.0, 1.0, 0, bin_dir, work_dir, 0, 0, sandbox);

    auto comp = buildDefaultComponent(fbb, "bad_supp_gid_comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

// ============================================================================
// Missing required field tests
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, MissingSchemaVersionReturnsInvalidFormat)
{
    RecordProperty("Description", "A config without schema_version returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto irt = fbb.CreateString("Startup");
    auto fallback = fb::CreateFallbackRunTarget(fbb, 0, 0, 1.0);
    auto alive_sup = fb::CreateAliveSupervision(fbb, 1.0);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto config = fb::CreateLaunchManagerConfig(fbb, std::nullopt, comps, rts, irt, fallback, alive_sup);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingEvaluationCycleReturnsInvalidFormat)
{
    RecordProperty("Description", "AliveSupervision without evaluation_cycle returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto irt = fbb.CreateString("Startup");
    auto fallback = fb::CreateFallbackRunTarget(fbb, 0, 0, 1.0);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto config = fb::CreateLaunchManagerConfig(fbb, 1, comps, rts, irt, fallback, alive_sup);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingFallbackTransitionTimeoutReturnsInvalidFormat)
{
    RecordProperty("Description", "FallbackRunTarget without transition_timeout returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto irt = fbb.CreateString("Startup");
    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb, 1.0);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto config = fb::CreateLaunchManagerConfig(fbb, 1, comps, rts, irt, fallback, alive_sup);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingRunTargetTransitionTimeoutReturnsInvalidFormat)
{
    RecordProperty("Description", "RunTarget without transition_timeout returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto switch_target = fbb.CreateString("SafeState");
    auto switch_action = fb::CreateSwitchRunTargetAction(fbb, switch_target);
    auto rt_name = fbb.CreateString("Startup");
    auto rt = fb::CreateRunTarget(fbb, rt_name, 0, 0, std::nullopt, switch_action);
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{rt});

    auto result = loadBuffer(buildConfigWithRunTargets(fbb, rts));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingReadyTimeoutReturnsInvalidFormat)
{
    RecordProperty("Description", "DeploymentConfig without ready_timeout returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(fbb, std::nullopt, 1.0, 0, bin_dir, work_dir, 0, 0, sandbox);

    auto comp = buildDefaultComponent(fbb, "comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingShutdownTimeoutReturnsInvalidFormat)
{
    RecordProperty("Description", "DeploymentConfig without shutdown_timeout returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(fbb, 1.0, std::nullopt, 0, bin_dir, work_dir, 0, 0, sandbox);

    auto comp = buildDefaultComponent(fbb, "comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingApplicationTypeReturnsInvalidFormat)
{
    RecordProperty("Description", "ApplicationProfile without application_type returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto app_profile = fb::CreateApplicationProfile(fbb, std::nullopt, false);
    auto bin_name = fbb.CreateString("bin");
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile, 0, 0, ready_cond);

    auto comp = buildDefaultComponent(fbb, "comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingIsSelfTerminatingReturnsInvalidFormat)
{
    RecordProperty("Description", "ApplicationProfile without is_self_terminating returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, std::nullopt);
    auto bin_name = fbb.CreateString("bin");
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile, 0, 0, ready_cond);

    auto comp = buildDefaultComponent(fbb, "comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingProcessStateReturnsInvalidFormat)
{
    RecordProperty("Description", "ReadyCondition without process_state returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false);
    auto bin_name = fbb.CreateString("bin");
    auto ready_cond = fb::CreateReadyCondition(fbb);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile, 0, 0, ready_cond);

    auto comp = buildDefaultComponent(fbb, "comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingReportingCycleReturnsInvalidFormat)
{
    RecordProperty("Description", "ComponentAliveSupervision without reporting_cycle returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto comp_alive_sup = fb::CreateComponentAliveSupervision(fbb, std::nullopt, 3);
    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false, comp_alive_sup);
    auto bin_name = fbb.CreateString("bin");
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile, 0, 0, ready_cond);

    auto comp = buildDefaultComponent(fbb, "comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingFailedCyclesToleranceReturnsInvalidFormat)
{
    RecordProperty("Description", "ComponentAliveSupervision without failed_cycles_tolerance returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto comp_alive_sup = fb::CreateComponentAliveSupervision(fbb, 1.0, std::nullopt);
    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false, comp_alive_sup);
    auto bin_name = fbb.CreateString("bin");
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile, 0, 0, ready_cond);

    auto comp = buildDefaultComponent(fbb, "comp", comp_props);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingRestartActionFieldsReturnsInvalidFormat)
{
    RecordProperty("Description", "RestartAction without required fields returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto restart = fb::CreateRestartAction(fbb);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(fbb, 1.0, 1.0, 0, bin_dir, work_dir, restart, 0, sandbox);

    auto comp = buildDefaultComponent(fbb, "comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingSandboxUidReturnsInvalidFormat)
{
    RecordProperty("Description", "Sandbox without uid returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto sandbox = fb::CreateSandbox(fbb, std::nullopt, 0, 0, 0, fb::SchedulingPolicy::OTHER, 0);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb, 1.0, 1.0, 0, bin_dir, work_dir, 0, 0, sandbox);

    auto comp = buildDefaultComponent(fbb, "comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingSandboxGidReturnsInvalidFormat)
{
    RecordProperty("Description", "Sandbox without gid returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto sandbox = fb::CreateSandbox(fbb, 0, std::nullopt, 0, 0, fb::SchedulingPolicy::OTHER, 0);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb, 1.0, 1.0, 0, bin_dir, work_dir, 0, 0, sandbox);

    auto comp = buildDefaultComponent(fbb, "comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingSandboxSchedulingPolicyReturnsInvalidFormat)
{
    RecordProperty("Description", "Sandbox without scheduling_policy returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto sandbox = fb::CreateSandbox(fbb, 0, 0, 0, 0, std::nullopt, 0);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb, 1.0, 1.0, 0, bin_dir, work_dir, 0, 0, sandbox);

    auto comp = buildDefaultComponent(fbb, "comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingSandboxSchedulingPriorityReturnsInvalidFormat)
{
    RecordProperty("Description", "Sandbox without scheduling_priority returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto sandbox = fb::CreateSandbox(fbb, 0, 0, 0, 0, fb::SchedulingPolicy::OTHER, std::nullopt);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb, 1.0, 1.0, 0, bin_dir, work_dir, 0, 0, sandbox);

    auto comp = buildDefaultComponent(fbb, "comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingWatchdogMaxTimeoutReturnsInvalidFormat)
{
    RecordProperty("Description", "Watchdog without max_timeout returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto dev_path = fbb.CreateString("/dev/watchdog0");
    auto watchdog = fb::CreateWatchdog(fbb, dev_path, std::nullopt, true, false);

    auto fallback = fb::CreateFallbackRunTarget(fbb, 0, 0, 1.0);
    auto alive_sup = fb::CreateAliveSupervision(fbb, 1.0);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb, 1, comps, rts, irt, fallback, alive_sup, watchdog);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingWatchdogDeactivateOnShutdownReturnsInvalidFormat)
{
    RecordProperty("Description", "Watchdog without deactivate_on_shutdown returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto dev_path = fbb.CreateString("/dev/watchdog0");
    auto watchdog = fb::CreateWatchdog(fbb, dev_path, 30.0, std::nullopt, false);

    auto fallback = fb::CreateFallbackRunTarget(fbb, 0, 0, 1.0);
    auto alive_sup = fb::CreateAliveSupervision(fbb, 1.0);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb, 1, comps, rts, irt, fallback, alive_sup, watchdog);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingWatchdogRequireMagicCloseReturnsInvalidFormat)
{
    RecordProperty("Description", "Watchdog without require_magic_close returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto dev_path = fbb.CreateString("/dev/watchdog0");
    auto watchdog = fb::CreateWatchdog(fbb, dev_path, 30.0, true, std::nullopt);

    auto fallback = fb::CreateFallbackRunTarget(fbb, 0, 0, 1.0);
    auto alive_sup = fb::CreateAliveSupervision(fbb, 1.0);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{});
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{});
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb, 1, comps, rts, irt, fallback, alive_sup, watchdog);

    auto result = loadBuffer(finishBuffer(fbb, config));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

// ============================================================================
// Parameterized scheduling policy tests
// ============================================================================

struct SchedulingPolicyTestParam
{
    fb::SchedulingPolicy fb_policy;
    int32_t expected_posix_value;
    const char* name;
};

class SchedulingPolicyTest : public FlatbufferConfigLoaderTest,
                             public ::testing::WithParamInterface<SchedulingPolicyTestParam>
{
};

TEST_P(SchedulingPolicyTest, ConvertsToExpectedPosixValue)
{
    RecordProperty("Description", "Scheduling policy enum maps to correct POSIX value.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto sandbox = fb::CreateSandbox(fbb,
                                     1000 /*uid*/,
                                     1000 /*gid*/,
                                     0 /*supplementary_group_ids*/,
                                     0 /*security_policy*/,
                                     GetParam().fb_policy,
                                     0 /*scheduling_priority*/);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb, 1.0, 1.0, 0, bin_dir, work_dir, 0, 0, sandbox);

    auto comp = buildDefaultComponent(fbb, "sched_comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->components()[0].deployment_config.sandbox.scheduling_policy,
                Eq(GetParam().expected_posix_value));
}

INSTANTIATE_TEST_SUITE_P(SchedulingPolicies,
                         SchedulingPolicyTest,
                         ::testing::Values(SchedulingPolicyTestParam{fb::SchedulingPolicy::FIFO,
                                                                     SCHED_FIFO,
                                                                     "SCHED_FIFO"},
                                           SchedulingPolicyTestParam{fb::SchedulingPolicy::RR,
                                                                     SCHED_RR,
                                                                     "SCHED_RR"},
                                           SchedulingPolicyTestParam{fb::SchedulingPolicy::OTHER,
                                                                     SCHED_OTHER,
                                                                     "SCHED_OTHER"}),
                         [](const ::testing::TestParamInfo<SchedulingPolicyTestParam>& info) {
                             return info.param.name;
                         });

TEST_F(FlatbufferConfigLoaderTest, UnsupportedSchedulingPolicyReturnsInvalidFormat)
{
    RecordProperty("Description", "An unsupported scheduling policy value returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;

    auto sandbox = fb::CreateSandbox(fbb,
                                     1000 /*uid*/,
                                     1000 /*gid*/,
                                     0 /*supplementary_group_ids*/,
                                     0 /*security_policy*/,
                                     static_cast<fb::SchedulingPolicy>(99),
                                     0 /*scheduling_priority*/);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb, 1.0, 1.0, 0, bin_dir, work_dir, 0, 0, sandbox);

    auto comp = buildDefaultComponent(fbb, "bad_sched_comp", buildDefaultComponentProperties(fbb), deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});

    auto result = loadBuffer(buildConfigWithComponents(fbb, comps));

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

}  // namespace
}  // namespace score::launch_manager::config
