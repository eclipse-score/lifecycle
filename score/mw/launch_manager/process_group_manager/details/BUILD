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
    visibility = ["//score/mw/launch_manager/process_group_manager:__pkg__"],
    deps = [
        "//score/mw/launch_manager/configuration:configuration_manager",
        "//score/mw/launch_manager/control:control_client_channel",
        "//score/mw/launch_manager/osal:ipc_comms",
        "//score/mw/launch_manager/osal:semaphore",
        "//score/mw/launch_manager/process_group_manager:iprocess",
    ],
)

cc_library(
    name = "graph",
    hdrs = ["graph.hpp"],
    visibility = ["//score/mw/launch_manager/process_group_manager:__pkg__"],
    deps = [
        ":process_info_node",
        "//score/mw/launch_manager/common:identifier_hash",
        "//score/mw/launch_manager/configuration:configuration_manager",
        "//score/mw/launch_manager/control:control_client_channel",
        "//score/mw/launch_manager/osal:semaphore",
        "//score/mw/launch_manager/process_group_manager:iprocess",
    ],
)

cc_library(
    name = "safe_process_map",
    srcs = ["safe_process_map.cpp"],
    hdrs = ["safe_process_map.hpp"],
    visibility = ["//score/mw/launch_manager/process_group_manager:__pkg__"],
    deps = [
        ":process_info_node",
        "//score/mw/launch_manager/process_group_manager:iprocess",
    ],
)

cc_library(
    name = "os_handler",
    srcs = ["os_handler.cpp"],
    hdrs = ["os_handler.hpp"],
    visibility = ["//score/mw/launch_manager/process_group_manager:__pkg__"],
    deps = [
        ":safe_process_map",
        "//score/mw/launch_manager/common:log",
        "//score/mw/launch_manager/process_group_manager:iprocess",
    ],
)

cc_library(
    name = "process_launcher",
    srcs = ["process_launcher.cpp"],
    visibility = ["//score/mw/launch_manager/process_group_manager:__pkg__"],
    deps = [
        "//score/mw/launch_manager/common:log",
        "//score/mw/launch_manager/control:control_client_channel",
        "//score/mw/launch_manager/osal:ipc_comms",
        "//score/mw/launch_manager/osal:security_policy",
        "//score/mw/launch_manager/osal:set_affinity",
        "//score/mw/launch_manager/osal:set_groups",
        "//score/mw/launch_manager/osal:sys_exit",
        "//score/mw/launch_manager/process_group_manager:iprocess",
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
    visibility = ["//score/mw/launch_manager/process_group_manager:__pkg__"],
    deps = [
        ":graph",
        ":os_handler",
        ":process_info_node",
        ":process_launcher",
        ":safe_process_map",
        "//score/mw/launch_manager/common:log",
        "//score/mw/launch_manager/common/concurrency:mpmc_concurrent_queue",
        "//score/mw/launch_manager/common/concurrency:workerthread",
        "//score/mw/launch_manager/configuration:configuration_manager",
        "//score/mw/launch_manager/osal:ipc_comms",
        "//score/mw/launch_manager/osal:semaphore",
        "//score/mw/launch_manager/process_group_manager:process_group_manager_hdrs",
        "//score/mw/launch_manager/process_state_client",
        "//score/mw/launch_manager/recovery_client",
        "@score_baselibs//score/language/futurecpp",
    ],
)
