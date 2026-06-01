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

cc_library(
    name = "process_info_node",
    hdrs = ["process_info_node.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/daemon/src/process_group_manager:__pkg__"],
    deps = [
        "//score/launch_manager/daemon/src/configuration:configuration_manager",
        "//score/launch_manager/daemon/src/control:control_client_channel",
        "//score/launch_manager/daemon/src/osal:ipc_comms",
        "//score/launch_manager/daemon/src/osal:semaphore",
        "//score/launch_manager/daemon/src/process_group_manager:iprocess",
    ],
)

cc_library(
    name = "graph",
    hdrs = ["graph.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":process_info_node",
        "//score/launch_manager/daemon/src/common:identifier_hash",
        "//score/launch_manager/daemon/src/configuration:configuration_manager",
        "//score/launch_manager/daemon/src/control:control_client_channel",
        "//score/launch_manager/daemon/src/osal:semaphore",
        "//score/launch_manager/daemon/src/process_group_manager:iprocess",
    ],
)

cc_library(
    name = "safe_process_map",
    srcs = ["safe_process_map.cpp"],
    hdrs = ["safe_process_map.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":process_info_node",
        "//score/launch_manager/daemon/src/process_group_manager:iprocess",
    ],
)

cc_library(
    name = "os_handler",
    srcs = ["os_handler.cpp"],
    hdrs = ["os_handler.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":safe_process_map",
        "//score/launch_manager/daemon/src/common:log",
        "//score/launch_manager/daemon/src/process_group_manager:iprocess",
    ],
)

cc_library(
    name = "process_launcher",
    srcs = ["process_launcher.cpp"],
    visibility = ["//score/launch_manager/daemon/src/process_group_manager:__pkg__"],
    deps = [
        "//score/launch_manager/daemon/src/common:log",
        "//score/launch_manager/daemon/src/control:control_client_channel",
        "//score/launch_manager/daemon/src/osal:ipc_comms",
        "//score/launch_manager/daemon/src/osal:security_policy",
        "//score/launch_manager/daemon/src/osal:set_affinity",
        "//score/launch_manager/daemon/src/osal:set_groups",
        "//score/launch_manager/daemon/src/osal:sys_exit",
        "//score/launch_manager/daemon/src/process_group_manager:iprocess",
    ],
)

# graph.cpp, process_info_node.cpp, and process_group_manager.cpp include
# process_group_manager.hpp which in turn includes graph.hpp and process_info_node.hpp —
# circular at link time, so all three must share a target.
cc_library(
    name = "process_group_manager_impl",
    srcs = [
        "graph.cpp",
        "process_group_manager.cpp",
        "process_info_node.cpp",
    ],
    visibility = ["//score/launch_manager/daemon/src/process_group_manager:__pkg__"],
    deps = [
        ":graph",
        ":os_handler",
        ":process_info_node",
        ":process_launcher",
        ":safe_process_map",
        "//score/launch_manager/daemon/src/common:log",
        "//score/launch_manager/daemon/src/common/concurrency:mpmc_concurrent_queue",
        "//score/launch_manager/daemon/src/common/concurrency:workerthread",
        "//score/launch_manager/daemon/src/configuration:configuration_manager",
        "//score/launch_manager/daemon/src/osal:ipc_comms",
        "//score/launch_manager/daemon/src/osal:semaphore",
        "//score/launch_manager/daemon/src/process_group_manager:process_group_manager_hdrs",
        "//score/launch_manager/daemon/src/process_state_client",
        "//score/launch_manager/daemon/src/recovery_client",
        "@score_baselibs//score/language/futurecpp",
    ],
)
