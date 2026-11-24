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
    name = "component",
    srcs = ["component.cpp"],
    hdrs = ["component.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":handle",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/alive_monitor:ialive_supervision_handle",
        "//score/launch_manager/src/daemon/src/alive_monitor:isupervision_factory",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "//score/launch_manager/src/daemon/src/configuration:environment_config",
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/force_stop_action:iforce_stop_action",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/ready_condition:iready_condition",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/start_action:istart_action",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/stop_action:istop_action",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
        "@score_baselibs//score/result:error",
    ],
)

lm_cc_test(
    name = "component_UT",
    srcs = ["component_UT.cpp"],
    deps = [
        ":component",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/alive_monitor:mock_alive_supervision_handle",
        "//score/launch_manager/src/daemon/src/alive_monitor:mock_supervision_factory",
        "//score/launch_manager/src/daemon/src/process_group_manager:mock_component",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/force_stop_action:mock_force_stop_action",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:mock_iprocess",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:process_launcher",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:process_terminated_ready_condition",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:report_running_ready_condition",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/ready_condition:mock_ready_condition",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/start_action:mock_start_action",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/stop_action:mock_stop_action",
        "@googletest//:gtest_main",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "component_factory",
    srcs = ["component_factory.cpp"],
    hdrs = ["component_factory.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":component",
        "//score/launch_manager/src/daemon/src/alive_monitor:isupervision_factory",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "//score/launch_manager/src/daemon/src/osal:ifile_waiter",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:iprocess",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:process_force_stop_action",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:process_start_action",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:process_stop_action",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:process_terminated_ready_condition",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:report_running_ready_condition",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:safe_process_map",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/ready_condition:empty_ready_condition",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/ready_condition:file_ready_condition",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/ready_condition:iready_condition",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/ready_condition:sequence_ready_condition",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
    ],
)

lm_cc_test(
    name = "component_factory_UT",
    srcs = ["component_factory_UT.cpp"],
    deps = [
        ":component_factory",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/alive_monitor:mock_alive_supervision_handle",
        "//score/launch_manager/src/daemon/src/alive_monitor:mock_supervision_factory",
        "//score/launch_manager/src/daemon/src/osal:mock_ifile_waiter",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:mock_iprocess",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:mock_safe_process_map_inserter",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "component_task",
    hdrs = ["component_task.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "icomponent_controller",
    hdrs = ["icomponent_controller.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":component_task",
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
    ],
)

cc_library(
    name = "handle",
    srcs = ["handle.cpp"],
    hdrs = ["handle.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
    ],
)

lm_cc_test(
    name = "handle_UT",
    srcs = ["handle_UT.cpp"],
    deps = [
        ":handle",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "mock_component_event_queue",
    testonly = True,
    hdrs = ["mock_component_event_queue.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":icomponent_event_publisher_consumer",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "mock_component_controller",
    testonly = True,
    hdrs = ["mock_component_controller.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":icomponent_controller",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "completion_slot",
    hdrs = ["completion_slot.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        "@score_baselibs//score/concurrency:notification",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "component_event",
    hdrs = ["component_event.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":completion_slot",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "component_event_queue",
    hdrs = ["component_event_queue.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":component_event",
        ":icomponent_event_publisher_consumer",
        "//score/launch_manager/src/daemon/src/common/concurrency:concurrency_error_domain",
        "//score/launch_manager/src/daemon/src/common/concurrency:mpsc_bounded_queue",
    ],
)

cc_library(
    name = "icomponent_event_publisher_consumer",
    hdrs = ["icomponent_event_publisher_consumer.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [":component_event"],
)

lm_cc_test(
    name = "completion_slot_UT",
    srcs = ["completion_slot_UT.cpp"],
    deps = [
        ":completion_slot",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "@googletest//:gtest_main",
        "@score_baselibs//score/result",
    ],
)

lm_cc_test(
    name = "component_event_queue_UT",
    srcs = ["component_event_queue_UT.cpp"],
    deps = [
        ":component_event_queue",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "graph",
    srcs = ["graph.cpp"],
    hdrs = ["graph.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = [
        "//score/launch_manager/src/daemon/src/process_group_manager:__pkg__",
    ],
    deps = [
        ":component",
        ":component_event",
        ":component_factory",
        ":component_of",
        ":component_task",
        ":dependency_graph",
        ":transition",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/common/concurrency:concurrency_error_domain",
        "//score/launch_manager/src/daemon/src/common/concurrency:mpmc_concurrent_queue",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "//score/launch_manager/src/daemon/src/configuration:config",
        "//score/launch_manager/src/daemon/src/osal:semaphore",
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
        "//score/launch_manager/src/daemon/src/process_group_manager:irun_target_control",
        "//score/launch_manager/src/daemon/src/process_group_manager:run_target",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:iprocess",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:process_handling",
        "//score/launch_manager/src/lm_control",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
        "@score_baselibs//score/result:error",
    ],
)

lm_cc_test(
    name = "graph_UT",
    srcs = ["graph_UT.cpp"],
    deps = [
        ":graph",
        "//score/launch_manager/src/daemon/src/alive_monitor:mock_alive_supervision_handle",
        "//score/launch_manager/src/daemon/src/alive_monitor:mock_supervision_factory",
        "//score/launch_manager/src/daemon/src/configuration:config",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/osal:mock_ifile_waiter",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:mock_iprocess",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "transition",
    hdrs = ["transition.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":component_of",
        ":dependency_graph",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common/concurrency:fixed_size_queue",
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "component_of",
    hdrs = ["component_of.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":component",
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
        "//score/launch_manager/src/daemon/src/process_group_manager:run_target",
    ],
)

lm_cc_test(
    name = "process_group_manager_UT",
    srcs = ["process_group_manager_UT.cpp"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor:mock_alive_monitor",
        "//score/launch_manager/src/daemon/src/alive_monitor:mock_alive_supervision_handle",
        "//score/launch_manager/src/daemon/src/alive_monitor:mock_supervision_factory",
        "//score/launch_manager/src/daemon/src/configuration:config",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/process_group_manager",
        "//score/launch_manager/src/daemon/src/process_group_manager/details/process:mock_iprocess",
        "//score/launch_manager/src/daemon/src/recovery_client:mock_recovery_client",
        "//score/launch_manager/src/daemon/src/watchdog:mock_i_watchdog_if",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "dependency_graph",
    hdrs = [
        "dependency_graph.hpp",
    ],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__pkg__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common/concurrency:fixed_size_queue",
        "@score_baselibs//score/language/futurecpp",
    ],
)

lm_cc_test(
    name = "dependency_graph_UT",
    srcs = ["dependency_graph_UT.cpp"],
    deps = [
        ":dependency_graph",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "@googletest//:gtest_main",
    ],
)

lm_cc_test(
    name = "transition_UT",
    srcs = ["transition_UT.cpp"],
    deps = [
        ":dependency_graph",
        ":transition",
        "//score/launch_manager/src/daemon/src/process_group_manager:icomponent",
        "@googletest//:gtest_main",
    ],
)
