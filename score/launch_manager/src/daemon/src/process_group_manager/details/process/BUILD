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
    name = "mock_safe_process_map_inserter",
    testonly = True,
    hdrs = ["mock_safe_process_map_inserter.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":safe_process_map",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "process_handling",
    hdrs = ["process_handling.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":iprocess",
        ":safe_process_map",
        "//score/launch_manager/src/daemon/src/alive_monitor:isupervision_factory",
        "//score/launch_manager/src/daemon/src/osal:ifile_waiter",
    ],
)

cc_library(
    name = "safe_process_map",
    srcs = ["safe_process_map.cpp"],
    hdrs = ["safe_process_map.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common:init_only_resource",
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:handle",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:icomponent_controller",
        "@score_baselibs//score/language/futurecpp",
    ],
)

lm_cc_test(
    name = "safeprocessmap_UT",
    srcs = ["safeprocessmap_UT.cpp"],
    deps = [
        ":safe_process_map",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:mock_component_controller",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "os_handler",
    srcs = ["os_handler.cpp"],
    hdrs = ["os_handler.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":safe_process_map",
        "//score/launch_manager/src/daemon/src/common:log",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/os:sys_wait",
        "@score_baselibs//score/os/utils:thread",
    ],
)

cc_library(
    name = "process_monitor",
    srcs = ["process_monitor.cpp"],
    hdrs = ["process_monitor.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:component_event",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:component_task",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:handle",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:icomponent_controller",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:icomponent_event_publisher_consumer",
        "@score_baselibs//score/language/futurecpp",
    ],
)

lm_cc_test(
    name = "process_monitor_UT",
    srcs = ["process_monitor_UT.cpp"],
    deps = [
        ":process_monitor",
        "//score/launch_manager/src/daemon/src/process_group_manager:mock_component",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:mock_component_event_queue",
        "@googletest//:gtest_main",
    ],
)

lm_cc_test(
    name = "oshandler_UT",
    srcs = ["oshandler_UT.cpp"],
    deps = [
        ":os_handler",
        ":safe_process_map",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/process_group_manager:mock_component",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:mock_component_controller",
        "@googletest//:gtest_main",
        "@score_baselibs//score/os/mocklib:sys_wait_mock",
    ],
)

cc_library(
    name = "process_launcher",
    srcs = ["process_launcher.cpp"],
    hdrs = ["process_launcher.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":iprocess",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/common:signal_safe_log",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "//score/launch_manager/src/daemon/src/configuration:environment_config",
        "//score/launch_manager/src/daemon/src/configuration:process_command",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "//score/launch_manager/src/daemon/src/osal:security_policy",
        "//score/launch_manager/src/daemon/src/osal:semaphore",
        "//score/launch_manager/src/daemon/src/osal:set_affinity",
        "//score/launch_manager/src/daemon/src/osal:set_groups",
        "//score/launch_manager/src/daemon/src/osal:shm_open_anon",
        "//score/launch_manager/src/daemon/src/osal:sys_exit",
        "@score_baselibs//score/language/futurecpp",
    ],
)

lm_cc_test(
    name = "process_launcher_UT",
    srcs = ["process_launcher_UT.cpp"],
    linkopts = [
        "-Wl,--wrap=fork",
        "-Wl,--wrap=execve",
        "-Wl,--wrap=kill",
        "-Wl,--wrap=wait",
        "-Wl,--wrap=access",
        "-Wl,--wrap=shm_open",
        "-Wl,--wrap=ftruncate",
        "-Wl,--wrap=shm_unlink",
        "-Wl,--wrap=mmap",
        "-Wl,--wrap=munmap",
        "-Wl,--wrap=setpgid",
        "-Wl,--wrap=setgid",
        "-Wl,--wrap=setuid",
        "-Wl,--wrap=sched_setscheduler",
        "-Wl,--wrap=chdir",
        "-Wl,--wrap=setrlimit",
        "-Wl,--wrap=getpid",
        "-Wl,--wrap=fcntl",
        "-Wl,--wrap=sem_destroy",
        "-Wl,--wrap=msync",
        "-Wl,--wrap=close",
        "-Wl,--wrap=fstat",
        "-Wl,--wrap=clock_gettime",
        "-Wl,--wrap=clock_getres",
    ],
    linkstatic = True,
    deps = [
        ":process_launcher",
        "//tests/utils/mocks:mock_syscalls",
        "@googletest//:gtest_main",
        "@score_baselibs//score/os/mocklib:semaphore_mock",
    ],
)

cc_library(
    name = "process_start_action",
    srcs = ["process_start_action.cpp"],
    hdrs = ["process_start_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":iprocess",
        ":safe_process_map",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:handle",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/start_action:istart_action",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
    ],
)

lm_cc_test(
    name = "process_start_action_UT",
    srcs = ["process_start_action_UT.cpp"],
    deps = [
        ":mock_iprocess",
        ":mock_safe_process_map_inserter",
        ":process_start_action",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/process_group_manager:mock_component",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "process_stop_action",
    srcs = ["process_stop_action.cpp"],
    hdrs = ["process_stop_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":iprocess",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:handle",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/stop_action:istop_action",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
    ],
)

lm_cc_test(
    name = "process_stop_action_UT",
    srcs = ["process_stop_action_UT.cpp"],
    deps = [
        ":mock_iprocess",
        ":process_stop_action",
        "//score/launch_manager:error",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "process_force_stop_action",
    srcs = ["process_force_stop_action.cpp"],
    hdrs = ["process_force_stop_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":iprocess",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:handle",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/force_stop_action:iforce_stop_action",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
    ],
)

lm_cc_test(
    name = "process_force_stop_action_UT",
    srcs = ["process_force_stop_action_UT.cpp"],
    deps = [
        ":mock_iprocess",
        ":process_force_stop_action",
        "//score/launch_manager:error",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "report_running_ready_condition",
    srcs = ["report_running_ready_condition.cpp"],
    hdrs = ["report_running_ready_condition.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":iprocess",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "//score/launch_manager/src/daemon/src/osal:semaphore",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:handle",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/ready_condition:iready_condition",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
    ],
)

lm_cc_test(
    name = "report_running_ready_condition_UT",
    srcs = ["report_running_ready_condition_UT.cpp"],
    deps = [
        ":mock_iprocess",
        ":report_running_ready_condition",
        "//score/launch_manager:error",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "termination_waiter",
    srcs = ["termination_waiter.cpp"],
    hdrs = ["termination_waiter.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        "@score_baselibs//score/concurrency:condition_variable",
        "@score_baselibs//score/language/futurecpp",
    ],
)

lm_cc_test(
    name = "termination_waiter_UT",
    srcs = ["termination_waiter_UT.cpp"],
    deps = [
        ":termination_waiter",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "process_terminated_ready_condition",
    srcs = ["process_terminated_ready_condition.cpp"],
    hdrs = ["process_terminated_ready_condition.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":termination_waiter",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:handle",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/ready_condition:iready_condition",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
    ],
)

lm_cc_test(
    name = "process_terminated_ready_condition_UT",
    srcs = ["process_terminated_ready_condition_UT.cpp"],
    deps = [
        ":process_terminated_ready_condition",
        "//score/launch_manager:error",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "iprocess",
    hdrs = ["iprocess.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
    ],
)

cc_library(
    name = "mock_iprocess",
    testonly = True,
    hdrs = ["mock_iprocess.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":iprocess",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "process_state",
    hdrs = ["process_state.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/process",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/process",
    visibility = ["//score:__subpackages__"],
)
