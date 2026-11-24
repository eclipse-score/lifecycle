# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
load("@rules_cc//cc:defs.bzl", "cc_library")
load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")

cc_library(
    name = "process_group_manager",
    srcs = ["process_group_manager.cpp"],
    hdrs = ["process_group_manager.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":irun_target_control",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/alive_monitor:i_alive_monitor",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/common/concurrency:concurrency_error_domain",
        "//score/launch_manager/src/daemon/src/common/concurrency:mpmc_concurrent_queue",
        "//score/launch_manager/src/daemon/src/common/concurrency:mpsc_bounded_queue",
        "//score/launch_manager/src/daemon/src/common/concurrency:thread_pool",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "//score/launch_manager/src/daemon/src/configuration:config",
        "//score/launch_manager/src/daemon/src/configuration:watchdog_config",
        "//score/launch_manager/src/daemon/src/osal:wait_for_file",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:completion_slot",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:component_event",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:component_event_queue",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:component_task",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:graph",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:iprocess",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:os_handler",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:process_handling",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:process_launcher",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:process_monitor",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:safe_process_map",
        "//score/launch_manager/src/daemon/src/recovery_client:irecovery_client",
        "//score/launch_manager/src/daemon/src/watchdog:i_watchdog_if",
        "//score/launch_manager/src/lm_control",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
        "@score_baselibs//score/result:error",
    ],
)

cc_library(
    name = "irun_target_control",
    hdrs = ["irun_target_control.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager",
    visibility = ["//score/launch_manager/src/daemon/src:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "icomponent",
    hdrs = ["icomponent.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:handle",
        "@score_baselibs//score/language/futurecpp",
    ],
)

lm_cc_test(
    name = "icomponent_UT",
    srcs = ["icomponent_UT.cpp"],
    deps = [
        ":icomponent",
        ":mock_component",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "mock_component",
    testonly = True,
    hdrs = ["mock_component.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":icomponent",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "run_target",
    hdrs = ["run_target.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":icomponent",
    ],
)
