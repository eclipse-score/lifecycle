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

#include "flatbuffer_config_loader.hpp"
#include "new_lm_flatcfg_generated.h"

#include "score/filesystem/path.h"
#include "score/os/ObjectSeam.h"
#include "score/os/mocklib/fcntl_mock.h"
#include "score/os/mocklib/stat_mock.h"
#include "score/os/mocklib/unistdmock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <vector>

namespace score::launch_manager::config {
namespace {

namespace fb = score::launch_manager::config::fb;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::NiceMock;
using ::testing::Return;

constexpr std::int32_t kTestFd = 42;
const score::filesystem::Path kTestPath{"/tmp/test_config.bin"};

std::vector<uint8_t> finishBuffer(::flatbuffers::FlatBufferBuilder& fbb,
                                  ::flatbuffers::Offset<fb::LaunchManagerConfig> root)
{
    fb::FinishLaunchManagerConfigBuffer(fbb, root);
    const auto* buf = fbb.GetBufferPointer();
    return {buf, buf + fbb.GetSize()};
}

class FlatbufferConfigLoaderTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");
    }

    void setUpSuccessfulFileRead(const std::vector<uint8_t>& content)
    {
        ON_CALL(*fcntl_mock_, open(_, _))
            .WillByDefault(Return(score::cpp::expected<std::int32_t, score::os::Error>{kTestFd}));

        ON_CALL(*stat_mock_, fstat(kTestFd, _))
            .WillByDefault(
                DoAll(Invoke([size = static_cast<std::int64_t>(content.size())](std::int32_t,
                                                                               score::os::StatBuffer& buf) {
                          buf.st_size = size;
                      }),
                      Return(score::cpp::expected_blank<score::os::Error>{})));

        ON_CALL(*unistd_mock_, read(kTestFd, _, _))
            .WillByDefault(
                Invoke([&content](std::int32_t, void* buf, std::size_t count)
                           -> score::cpp::expected<ssize_t, score::os::Error> {
                    const auto bytes = std::min(count, content.size());
                    std::memcpy(buf, content.data(), bytes);
                    return static_cast<ssize_t>(bytes);
                }));

        ON_CALL(*unistd_mock_, close(kTestFd))
            .WillByDefault(Return(score::cpp::expected_blank<score::os::Error>{}));
    }

    std::vector<uint8_t> buildMinimalConfig(int32_t schema_version = 1,
                                            const char* initial_run_target = "Startup")
    {
        ::flatbuffers::FlatBufferBuilder fbb;
        auto irt = fbb.CreateString(initial_run_target);
        auto fallback = fb::CreateFallbackRunTarget(fbb);
        auto alive_sup = fb::CreateAliveSupervision(fbb);
        auto config = fb::CreateLaunchManagerConfig(
            fbb, schema_version, 0 /*components*/, 0 /*run_targets*/, irt, fallback, alive_sup);
        return finishBuffer(fbb, config);
    }

    score::os::MockGuard<NiceMock<score::os::FcntlMock>> fcntl_mock_;
    score::os::MockGuard<NiceMock<score::os::StatMock>> stat_mock_;
    score::os::MockGuard<NiceMock<score::os::UnistdMock>> unistd_mock_;
    FlatbufferConfigLoader loader_;
};

// ============================================================================
// Happy path tests
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, LoadMinimalConfig)
{
    RecordProperty("Description", "Loads a minimal config with only required fields.");

    // GIVEN
    auto buffer = buildMinimalConfig(1, "Startup");
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->initial_run_target(), Eq("Startup"));
    EXPECT_THAT(result->components().empty(), IsTrue());
    EXPECT_THAT(result->run_targets().empty(), IsTrue());
    EXPECT_THAT(result->fallback_run_target().has_value(), IsTrue());
    EXPECT_THAT(result->alive_supervision().has_value(), IsTrue());
    EXPECT_THAT(result->watchdog().has_value(), IsFalse());
}

TEST_F(FlatbufferConfigLoaderTest, LoadSingleComponent)
{
    RecordProperty("Description", "Loads a config with one fully-populated component.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto alive_sup = fb::CreateComponentAliveSupervision(fbb,
        0.5 /*reporting_cycle*/, 2 /*failed_cycles_tolerance*/, 1 /*min_indications*/, 3 /*max_indications*/);
    auto app_profile =
        fb::CreateApplicationProfile(fbb, fb::ApplicationType_Reporting_And_Supervised, true /*is_self_terminating*/, alive_sup);
    auto bin_name = fbb.CreateString("my_binary");
    auto dep = fbb.CreateString("other_comp");
    auto deps_vec = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{dep});
    auto arg = fbb.CreateString("--verbose");
    auto args_vec = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{arg});
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState_Running);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile, deps_vec, args_vec, ready_cond);

    auto bin_dir = fbb.CreateString("/opt/bin");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb,
        1.5 /*ready_timeout*/, 2.5 /*shutdown_timeout*/, 0 /*environmental_variables*/, bin_dir, work_dir);

    auto comp_name = fbb.CreateString("TestComponent");
    auto comp_desc = fbb.CreateString("A test component");
    auto component = fb::CreateComponent(fbb, comp_name, comp_desc, comp_props, deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{component});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto global_alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, comps, 0 /*run_targets*/, irt, fallback, global_alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components().size(), Eq(1U));

    const auto& comp = result->components()[0];
    EXPECT_THAT(comp.name, Eq("TestComponent"));
    EXPECT_THAT(comp.description, Eq("A test component"));
    EXPECT_THAT(comp.component_properties.binary_name, Eq("my_binary"));
    EXPECT_THAT(comp.component_properties.application_profile.application_type, Eq(ApplicationType::ReportingAndSupervised));
    EXPECT_THAT(comp.component_properties.application_profile.is_self_terminating, IsTrue());
    ASSERT_THAT(comp.component_properties.application_profile.alive_supervision.has_value(), IsTrue());
    EXPECT_THAT(comp.component_properties.application_profile.alive_supervision->reporting_cycle_ms, Eq(500U));
    EXPECT_THAT(comp.component_properties.application_profile.alive_supervision->failed_cycles_tolerance, Eq(2U));
    EXPECT_THAT(comp.component_properties.application_profile.alive_supervision->min_indications, Eq(1U));
    EXPECT_THAT(comp.component_properties.application_profile.alive_supervision->max_indications, Eq(3U));
    ASSERT_THAT(comp.component_properties.depends_on.size(), Eq(1U));
    EXPECT_THAT(comp.component_properties.depends_on[0], Eq("other_comp"));
    ASSERT_THAT(comp.component_properties.process_arguments.size(), Eq(1U));
    EXPECT_THAT(comp.component_properties.process_arguments[0], Eq("--verbose"));
    EXPECT_THAT(comp.component_properties.ready_condition.process_state, Eq(ProcessState::Running));
    EXPECT_THAT(comp.deployment_config.ready_timeout_ms, Eq(1500U));
    EXPECT_THAT(comp.deployment_config.shutdown_timeout_ms, Eq(2500U));
    EXPECT_THAT(comp.deployment_config.bin_dir, Eq("/opt/bin"));
    EXPECT_THAT(comp.deployment_config.working_dir, Eq("/tmp"));
}

TEST_F(FlatbufferConfigLoaderTest, LoadRunTargets)
{
    RecordProperty("Description", "Loads run targets with dependencies and transition timeout.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto switch_target = fbb.CreateString("SafeState");
    auto switch_action = fb::CreateSwitchRunTargetAction(fbb, switch_target);
    auto rt_name = fbb.CreateString("Startup");
    auto rt_desc = fbb.CreateString("Initial state");
    auto rt_dep = fbb.CreateString("component_a");
    auto rt_deps = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{rt_dep});
    auto rt = fb::CreateRunTarget(fbb, rt_name, rt_desc, rt_deps, 5.0 /*transition_timeout*/,
        fb::RecoveryAction_SwitchRunTargetAction, switch_action.Union());
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{rt});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, 0 /*components*/, rts, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->run_targets().size(), Eq(1U));

    const auto& target = result->run_targets()[0];
    EXPECT_THAT(target.name, Eq("Startup"));
    EXPECT_THAT(target.description, Eq("Initial state"));
    ASSERT_THAT(target.depends_on.size(), Eq(1U));
    EXPECT_THAT(target.depends_on[0], Eq("component_a"));
    EXPECT_THAT(target.transition_timeout_ms, Eq(5000U));
    ASSERT_THAT(target.recovery_action.has_value(), IsTrue());
    ASSERT_THAT(std::holds_alternative<SwitchRunTargetAction>(target.recovery_action.value()), IsTrue());
    EXPECT_THAT(std::get<SwitchRunTargetAction>(target.recovery_action.value()).run_target, Eq("SafeState"));
}

TEST_F(FlatbufferConfigLoaderTest, LoadFallbackRunTarget)
{
    RecordProperty("Description", "Loads fallback run target with all fields.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto fb_desc = fbb.CreateString("Fallback state");
    auto fb_dep = fbb.CreateString("critical_comp");
    auto fb_deps = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{fb_dep});
    auto fallback = fb::CreateFallbackRunTarget(fbb, fb_desc, fb_deps, 10.0 /*transition_timeout*/);

    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, 0 /*components*/, 0 /*run_targets*/, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->fallback_run_target().has_value(), IsTrue());

    const auto& fb = result->fallback_run_target().value();
    EXPECT_THAT(fb.description, Eq("Fallback state"));
    ASSERT_THAT(fb.depends_on.size(), Eq(1U));
    EXPECT_THAT(fb.depends_on[0], Eq("critical_comp"));
    EXPECT_THAT(fb.transition_timeout_ms, Eq(10000U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadAliveSupervision)
{
    RecordProperty("Description", "Loads global alive supervision config.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto alive_sup = fb::CreateAliveSupervision(fbb, 0.25 /*evaluation_cycle*/);
    auto fallback = fb::CreateFallbackRunTarget(fbb);

    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, 0 /*components*/, 0 /*run_targets*/, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->alive_supervision().has_value(), IsTrue());
    EXPECT_THAT(result->alive_supervision()->evaluation_cycle_ms, Eq(250U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadWatchdog)
{
    RecordProperty("Description", "Loads watchdog config with all fields.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto dev_path = fbb.CreateString("/dev/watchdog0");
    auto watchdog = fb::CreateWatchdog(fbb, dev_path,
        30.0 /*max_timeout*/, true /*deactivate_on_shutdown*/, false /*require_magic_close*/);

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, 0 /*components*/, 0 /*run_targets*/, irt, fallback, alive_sup, watchdog);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
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
    RecordProperty("Description", "Recovery action variant holds RestartAction with correct fields.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto restart = fb::CreateRestartAction(fbb, 3 /*number_of_attempts*/, 1.5 /*delay_before_restart*/);
    auto rt_name = fbb.CreateString("Startup");
    auto rt = fb::CreateRunTarget(
        fbb, rt_name, 0 /*description*/, 0 /*depends_on*/, 0.0 /*transition_timeout*/,
        fb::RecoveryAction_RestartAction, restart.Union());
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{rt});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, 0 /*components*/, rts, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->run_targets().size(), Eq(1U));
    ASSERT_THAT(result->run_targets()[0].recovery_action.has_value(), IsTrue());

    const auto& action = result->run_targets()[0].recovery_action.value();
    ASSERT_THAT(std::holds_alternative<RestartAction>(action), IsTrue());

    const auto& ra = std::get<RestartAction>(action);
    EXPECT_THAT(ra.number_of_attempts, Eq(3U));
    EXPECT_THAT(ra.delay_before_restart_ms, Eq(1500U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadSwitchRunTargetAction)
{
    RecordProperty("Description", "Recovery action variant holds SwitchRunTargetAction.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto target_name = fbb.CreateString("Fallback");
    auto switch_action = fb::CreateSwitchRunTargetAction(fbb, target_name);
    auto rt_name = fbb.CreateString("Startup");
    auto rt = fb::CreateRunTarget(
        fbb, rt_name, 0 /*description*/, 0 /*depends_on*/, 0.0 /*transition_timeout*/,
        fb::RecoveryAction_SwitchRunTargetAction, switch_action.Union());
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{rt});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, 0 /*components*/, rts, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->run_targets()[0].recovery_action.has_value(), IsTrue());

    const auto& action = result->run_targets()[0].recovery_action.value();
    ASSERT_THAT(std::holds_alternative<SwitchRunTargetAction>(action), IsTrue());
    EXPECT_THAT(std::get<SwitchRunTargetAction>(action).run_target, Eq("Fallback"));
}

TEST_F(FlatbufferConfigLoaderTest, LoadSandbox)
{
    RecordProperty("Description", "Loads sandbox config including optional fields.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto sec_policy = fbb.CreateString("strict");
    auto sched_policy = fbb.CreateString("SCHED_FIFO");
    auto supp_gids = fbb.CreateVector(std::vector<uint32_t>{100, 200});
    auto sandbox = fb::CreateSandbox(fbb,
        1000 /*uid*/, 1000 /*gid*/, supp_gids, sec_policy, sched_policy,
        50 /*scheduling_priority*/, 4096 /*max_memory_usage*/, 80 /*max_cpu_usage*/);

    auto sb_bin_name = fbb.CreateString("sandboxed_bin");
    auto sb_app_profile = fb::CreateApplicationProfile(fbb);
    auto sb_ready_cond = fb::CreateReadyCondition(fbb);
    auto sb_comp_props = fb::CreateComponentProperties(fbb, sb_bin_name, sb_app_profile,
        0 /*depends_on*/, 0 /*process_arguments*/, sb_ready_cond);

    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb,
        0.5 /*ready_timeout*/, 0.5 /*shutdown_timeout*/, 0 /*environmental_variables*/, bin_dir, work_dir,
        fb::RecoveryAction_NONE, 0 /*ready_recovery_action*/,
        fb::RecoveryAction_NONE, 0 /*recovery_action*/, sandbox);

    auto comp_name = fbb.CreateString("sandboxed_comp");
    auto component = fb::CreateComponent(fbb, comp_name, 0 /*description*/, sb_comp_props, deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{component});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, comps, 0 /*run_targets*/, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components().size(), Eq(1U));
    ASSERT_THAT(result->components()[0].deployment_config.sandbox.has_value(), IsTrue());

    const auto& sb = result->components()[0].deployment_config.sandbox.value();
    EXPECT_THAT(sb.uid, Eq(1000U));
    EXPECT_THAT(sb.gid, Eq(1000U));
    EXPECT_THAT(sb.supplementary_group_ids, Eq(std::vector<uint32_t>{100, 200}));
    ASSERT_THAT(sb.security_policy.has_value(), IsTrue());
    EXPECT_THAT(sb.security_policy.value(), Eq("strict"));
    EXPECT_THAT(sb.scheduling_policy, Eq("SCHED_FIFO"));
    EXPECT_THAT(sb.scheduling_priority, Eq(50));
    ASSERT_THAT(sb.max_memory_usage.has_value(), IsTrue());
    EXPECT_THAT(sb.max_memory_usage.value(), Eq(4096U));
    ASSERT_THAT(sb.max_cpu_usage.has_value(), IsTrue());
    EXPECT_THAT(sb.max_cpu_usage.value(), Eq(80U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadComponentAliveSupervision)
{
    RecordProperty("Description", "Per-component alive supervision is mapped correctly.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto comp_alive_sup = fb::CreateComponentAliveSupervision(fbb,
        1.0 /*reporting_cycle*/, 3 /*failed_cycles_tolerance*/, 2 /*min_indications*/, 5 /*max_indications*/);
    auto app_profile = fb::CreateApplicationProfile(fbb,
        fb::ApplicationType_Reporting_And_Supervised, false /*is_self_terminating*/, comp_alive_sup);
    auto bin_name = fbb.CreateString("supervised_bin");
    auto ready_cond = fb::CreateReadyCondition(fbb);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile,
        0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);

    auto bin_dir = fbb.CreateString("/opt");
    auto deploy = fb::CreateDeploymentConfig(fbb,
        0.0 /*ready_timeout*/, 0.0 /*shutdown_timeout*/, 0 /*environmental_variables*/, bin_dir);

    auto comp_name = fbb.CreateString("supervised_comp");
    auto component = fb::CreateComponent(fbb, comp_name, 0 /*description*/, comp_props, deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{component});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, comps, 0 /*run_targets*/, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    const auto& as = result->components()[0].component_properties.application_profile.alive_supervision;
    ASSERT_THAT(as.has_value(), IsTrue());
    EXPECT_THAT(as->reporting_cycle_ms, Eq(1000U));
    EXPECT_THAT(as->failed_cycles_tolerance, Eq(3U));
    EXPECT_THAT(as->min_indications, Eq(2U));
    EXPECT_THAT(as->max_indications, Eq(5U));
}

TEST_F(FlatbufferConfigLoaderTest, LoadEnvironmentalVariables)
{
    RecordProperty("Description", "Environmental variables vector is mapped to unordered_map.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto k1 = fbb.CreateString("PATH");
    auto v1 = fbb.CreateString("/usr/bin");
    auto ev1 = fb::CreateEnvironmentalVariable(fbb, k1, v1);
    auto k2 = fbb.CreateString("HOME");
    auto v2 = fbb.CreateString("/root");
    auto ev2 = fb::CreateEnvironmentalVariable(fbb, k2, v2);
    auto env_vars = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::EnvironmentalVariable>>{ev1, ev2});

    auto env_bin_name = fbb.CreateString("env_bin");
    auto env_app_profile = fb::CreateApplicationProfile(fbb);
    auto env_ready_cond = fb::CreateReadyCondition(fbb);
    auto env_comp_props = fb::CreateComponentProperties(fbb, env_bin_name, env_app_profile,
        0 /*depends_on*/, 0 /*process_arguments*/, env_ready_cond);

    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb,
        0.5 /*ready_timeout*/, 0.5 /*shutdown_timeout*/, env_vars, bin_dir, work_dir);

    auto comp_name = fbb.CreateString("env_comp");
    auto component = fb::CreateComponent(fbb, comp_name, 0 /*description*/, env_comp_props, deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{component});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, comps, 0 /*run_targets*/, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    const auto& env = result->components()[0].deployment_config.environmental_variables;
    EXPECT_THAT(env.size(), Eq(2U));
    EXPECT_THAT(env.at("PATH"), Eq("/usr/bin"));
    EXPECT_THAT(env.at("HOME"), Eq("/root"));
}

TEST_F(FlatbufferConfigLoaderTest, LoadMultipleComponents)
{
    RecordProperty("Description", "Multiple components are all present and in order.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto make_component = [&fbb](const char* name) {
        auto comp_name = fbb.CreateString(name);
        auto bin_name = fbb.CreateString(name);
        auto app_profile = fb::CreateApplicationProfile(fbb);
        auto ready_cond = fb::CreateReadyCondition(fbb);
        auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile,
            0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);
        auto bin_dir = fbb.CreateString("/opt");
        auto deploy = fb::CreateDeploymentConfig(fbb,
            0.0 /*ready_timeout*/, 0.0 /*shutdown_timeout*/, 0 /*environmental_variables*/, bin_dir);
        return fb::CreateComponent(fbb, comp_name, 0 /*description*/, comp_props, deploy);
    };

    auto comp_a = make_component("CompA");
    auto comp_b = make_component("CompB");
    auto comp_c = make_component("CompC");
    auto comps = fbb.CreateVector(
        std::vector<::flatbuffers::Offset<fb::Component>>{comp_a, comp_b, comp_c});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, comps, 0 /*run_targets*/, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->components().size(), Eq(3U));
    EXPECT_THAT(result->components()[0].name, Eq("CompA"));
    EXPECT_THAT(result->components()[1].name, Eq("CompB"));
    EXPECT_THAT(result->components()[2].name, Eq("CompC"));
}

// ============================================================================
// Optional / required field tests
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, MissingRequiredFallbackReturnsInvalidFormat)
{
    RecordProperty("Description", "When the required fallback_run_target is absent, returns InvalidFormat.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;
    auto irt = fbb.CreateString("Startup");
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto config = fb::CreateLaunchManagerConfig(fbb, 1 /*schema_version*/, 0 /*components*/, 0 /*run_targets*/, irt,
        0 /*fallback_run_target*/, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingRequiredAliveSupervisionReturnsInvalidFormat)
{
    RecordProperty("Description", "When the required alive_supervision is absent, returns InvalidFormat.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;
    auto irt = fbb.CreateString("Startup");
    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto config = fb::CreateLaunchManagerConfig(fbb, 1 /*schema_version*/, 0 /*components*/, 0 /*run_targets*/, irt,
        fallback, 0 /*alive_supervision*/);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, OptionalWatchdogAbsent)
{
    RecordProperty("Description", "When no watchdog is present, it is nullopt.");

    // GIVEN
    auto buffer = buildMinimalConfig();
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->watchdog().has_value(), IsFalse());
}

TEST_F(FlatbufferConfigLoaderTest, OptionalSandboxAbsent)
{
    RecordProperty("Description", "When no sandbox is present on a component, it is nullopt.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto ns_bin_name = fbb.CreateString("no_sandbox_bin");
    auto ns_app_profile = fb::CreateApplicationProfile(fbb);
    auto ns_ready_cond = fb::CreateReadyCondition(fbb);
    auto ns_comp_props = fb::CreateComponentProperties(fbb, ns_bin_name, ns_app_profile,
        0 /*depends_on*/, 0 /*process_arguments*/, ns_ready_cond);

    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto deploy = fb::CreateDeploymentConfig(fbb,
        0.5 /*ready_timeout*/, 0.5 /*shutdown_timeout*/, 0 /*environmental_variables*/, bin_dir, work_dir);

    auto comp_name = fbb.CreateString("no_sandbox");
    auto component = fb::CreateComponent(fbb, comp_name, 0 /*description*/, ns_comp_props, deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{component});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, comps, 0 /*run_targets*/, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->components()[0].deployment_config.sandbox.has_value(), IsFalse());
}

TEST_F(FlatbufferConfigLoaderTest, MissingRequiredRecoveryActionReturnsInvalidFormat)
{
    RecordProperty("Description", "When the required recovery_action is absent on a RunTarget, returns InvalidFormat.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto rt_name = fbb.CreateString("Startup");
    auto rt = fb::CreateRunTarget(fbb, rt_name);
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{rt});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, 0 /*components*/, rts, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

// ============================================================================
// Enum mapping tests
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, MapNativeApplicationType)
{
    RecordProperty("Description", "ApplicationType_Native maps to ApplicationType::Native.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType_Native);
    auto bin_name = fbb.CreateString("native_bin");
    auto ready_cond = fb::CreateReadyCondition(fbb);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile,
        0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);
    auto bin_dir = fbb.CreateString("/opt");
    auto deploy = fb::CreateDeploymentConfig(fbb,
        0.0 /*ready_timeout*/, 0.0 /*shutdown_timeout*/, 0 /*environmental_variables*/, bin_dir);
    auto comp_name = fbb.CreateString("native_comp");
    auto component = fb::CreateComponent(fbb, comp_name, 0 /*description*/, comp_props, deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{component});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, comps, 0 /*run_targets*/, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->components()[0].component_properties.application_profile.application_type,
                Eq(ApplicationType::Native));
}

TEST_F(FlatbufferConfigLoaderTest, MapReportingAndSupervisedType)
{
    RecordProperty("Description",
                   "ApplicationType_Reporting_And_Supervised maps to ApplicationType::ReportingAndSupervised.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType_Reporting_And_Supervised);
    auto bin_name = fbb.CreateString("supervised_bin");
    auto ready_cond = fb::CreateReadyCondition(fbb);
    auto comp_props = fb::CreateComponentProperties(fbb, bin_name, app_profile,
        0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);
    auto bin_dir = fbb.CreateString("/opt");
    auto deploy = fb::CreateDeploymentConfig(fbb,
        0.0 /*ready_timeout*/, 0.0 /*shutdown_timeout*/, 0 /*environmental_variables*/, bin_dir);
    auto comp_name = fbb.CreateString("supervised_comp");
    auto component = fb::CreateComponent(fbb, comp_name, 0 /*description*/, comp_props, deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{component});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, comps, 0 /*run_targets*/, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->components()[0].component_properties.application_profile.application_type,
                Eq(ApplicationType::ReportingAndSupervised));
}

TEST_F(FlatbufferConfigLoaderTest, MapTerminatedProcessState)
{
    RecordProperty("Description", "ProcessState_Terminated maps to ProcessState::Terminated.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;

    auto app_profile = fb::CreateApplicationProfile(fbb);
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState_Terminated);
    auto bin_name = fbb.CreateString("term_bin");
    auto comp_props = fb::CreateComponentProperties(fbb,
        bin_name, app_profile, 0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);
    auto bin_dir = fbb.CreateString("/opt");
    auto deploy = fb::CreateDeploymentConfig(fbb,
        0.0 /*ready_timeout*/, 0.0 /*shutdown_timeout*/, 0 /*environmental_variables*/, bin_dir);
    auto comp_name = fbb.CreateString("term_comp");
    auto component = fb::CreateComponent(fbb, comp_name, 0 /*description*/, comp_props, deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{component});

    auto fallback = fb::CreateFallbackRunTarget(fbb);
    auto alive_sup = fb::CreateAliveSupervision(fbb);
    auto irt = fbb.CreateString("Startup");
    auto config = fb::CreateLaunchManagerConfig(fbb,
        1 /*schema_version*/, comps, 0 /*run_targets*/, irt, fallback, alive_sup);
    auto buffer = finishBuffer(fbb, config);
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->components()[0].component_properties.ready_condition.process_state,
                Eq(ProcessState::Terminated));
}

// ============================================================================
// Error path tests
// ============================================================================

TEST_F(FlatbufferConfigLoaderTest, FileOpenFailsReturnsFileNotFound)
{
    RecordProperty("Description", "When file open fails with ENOENT, returns FileNotFound error.");

    // GIVEN
    const auto open_error = score::os::Error::createFromErrno(ENOENT);
    EXPECT_CALL(*fcntl_mock_, open(_, _))
        .WillOnce(Return(score::cpp::make_unexpected(open_error)));

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::FileNotFound));
}

TEST_F(FlatbufferConfigLoaderTest, FileOpenFailsWithPermissionDeniedReturnsInsufficientPermission)
{
    RecordProperty("Description", "When file open fails with EACCES, returns InsufficientPermission error.");

    // GIVEN
    const auto open_error = score::os::Error::createFromErrno(EACCES);
    EXPECT_CALL(*fcntl_mock_, open(_, _))
        .WillOnce(Return(score::cpp::make_unexpected(open_error)));

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InsufficientPermission));
}

TEST_F(FlatbufferConfigLoaderTest, FstatFailsReturnsGeneralError)
{
    RecordProperty("Description", "When fstat fails, returns GeneralError.");

    // GIVEN
    ON_CALL(*fcntl_mock_, open(_, _))
        .WillByDefault(Return(score::cpp::expected<std::int32_t, score::os::Error>{kTestFd}));

    const auto stat_error = score::os::Error::createFromErrno(EBADF);
    EXPECT_CALL(*stat_mock_, fstat(kTestFd, _))
        .WillOnce(Return(score::cpp::make_unexpected(stat_error)));

    ON_CALL(*unistd_mock_, close(_))
        .WillByDefault(Return(score::cpp::expected_blank<score::os::Error>{}));

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::GeneralError));
}

TEST_F(FlatbufferConfigLoaderTest, ReadFailsReturnsGeneralError)
{
    RecordProperty("Description", "When read fails, returns GeneralError.");

    // GIVEN
    ON_CALL(*fcntl_mock_, open(_, _))
        .WillByDefault(Return(score::cpp::expected<std::int32_t, score::os::Error>{kTestFd}));

    ON_CALL(*stat_mock_, fstat(kTestFd, _))
        .WillByDefault(
            DoAll(Invoke([](std::int32_t, score::os::StatBuffer& buf) { buf.st_size = 100; }),
                  Return(score::cpp::expected_blank<score::os::Error>{})));

    const auto read_error = score::os::Error::createFromErrno(EIO);
    EXPECT_CALL(*unistd_mock_, read(kTestFd, _, _))
        .WillOnce(Return(score::cpp::make_unexpected(read_error)));

    ON_CALL(*unistd_mock_, close(_))
        .WillByDefault(Return(score::cpp::expected_blank<score::os::Error>{}));

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::GeneralError));
}

TEST_F(FlatbufferConfigLoaderTest, CorruptedBufferReturnsInvalidFormat)
{
    RecordProperty("Description", "Corrupted binary data returns InvalidFormat error.");

    // GIVEN
    std::vector<uint8_t> garbage = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01, 0x02, 0x03};
    setUpSuccessfulFileRead(garbage);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, MissingInitialRunTargetReturnsInvalidFormat)
{
    RecordProperty("Description", "A valid FlatBuffer with null initial_run_target returns InvalidFormat.");

    // GIVEN
    ::flatbuffers::FlatBufferBuilder fbb;
    auto config = fb::CreateLaunchManagerConfig(fbb, 1 /*schema_version*/);
    fb::FinishLaunchManagerConfigBuffer(fbb, config);
    const auto* buf = fbb.GetBufferPointer();
    std::vector<uint8_t> buffer(buf, buf + fbb.GetSize());
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(FlatbufferConfigLoaderTest, WrongSchemaVersionReturnsUnsupportedVersion)
{
    RecordProperty("Description", "A config with an unexpected schema_version returns UnsupportedVersion.");

    // GIVEN
    auto buffer = buildMinimalConfig(99, "Startup");
    setUpSuccessfulFileRead(buffer);

    // WHEN
    auto result = loader_.load(kTestPath);

    // THEN
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::UnsupportedVersion));
}

}  // namespace
}  // namespace score::launch_manager::config
