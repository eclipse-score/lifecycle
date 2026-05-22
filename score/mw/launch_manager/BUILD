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
load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "launch_manager",
    srcs = ["main.cpp"],
    linkopts = select({
        "@platforms//os:qnx": ["-lsecpol"],
        "@platforms//os:linux": ["-lpthread"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "//score/mw/launch_manager/alive_monitor",
        "//score/mw/launch_manager/common:log",
        "//score/mw/launch_manager/osal:ipc_comms",
        "//score/mw/launch_manager/process_group_manager",
        "//score/mw/launch_manager/process_group_manager:alive_monitor_thread",
        "//score/mw/launch_manager/process_state_client:process_state_notifier",
        "//score/mw/launch_manager/recovery_client",
    ],
)
