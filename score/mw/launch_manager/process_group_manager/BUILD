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
    name = "iprocess",
    hdrs = ["iprocess.hpp"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/mw/launch_manager/common:constants",
        "//score/mw/launch_manager/osal:ipc_comms",
    ],
)

cc_library(
    name = "ialive_monitor_thread",
    hdrs = ["ialive_monitor_thread.hpp"],
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "alive_monitor_thread",
    srcs = ["alive_monitor_thread.cpp"],
    hdrs = ["alive_monitor_thread.hpp"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":ialive_monitor_thread",
        "//score/mw/launch_manager/alive_monitor",
    ],
)

# Header-only target for process_group_manager.hpp; used by details:process_group_manager_impl
# to break the circular header dependency without creating a Bazel cycle.
cc_library(
    name = "process_group_manager_hdrs",
    hdrs = ["process_group_manager.hpp"],
    visibility = ["//score/mw/launch_manager/process_group_manager/details:__pkg__"],
    deps = [
        ":ialive_monitor_thread",
        ":iprocess",
        "//score/mw/launch_manager/common:identifier_hash",
        "//score/mw/launch_manager/common/concurrency:mpmc_concurrent_queue",
        "//score/mw/launch_manager/common/concurrency:workerthread",
        "//score/mw/launch_manager/configuration:configuration_manager",
        "//score/mw/launch_manager/control:control_client_channel",
        "//score/mw/launch_manager/osal:ipc_comms",
        "//score/mw/launch_manager/process_group_manager/details:graph",
        "//score/mw/launch_manager/process_group_manager/details:os_handler",
        "//score/mw/launch_manager/process_group_manager/details:process_info_node",
        "//score/mw/launch_manager/process_group_manager/details:safe_process_map",
        "//score/mw/launch_manager/process_state_client:iprocess_state_notifier",
        "//score/mw/launch_manager/recovery_client",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "process_group_manager",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":process_group_manager_hdrs",
        "//score/mw/launch_manager/process_group_manager/details:process_group_manager_impl",
    ],
)
