# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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
load("//config:common_cc.bzl", "cc_binary_with_common_opts", "cc_library_with_common_opts")

cc_library(
    name = "config",
    hdrs = ["config/lm_flatcfg_generated.h"],
    includes = ["config"],
    visibility = ["//src:__pkg__"],
)

cc_binary_with_common_opts(
    name = "launch_manager",
    srcs = glob(
        [
            "src/**/*.cpp",
            "src/**/*.hpp",
        ],
    ),
    includes = ["src/"],
    linkopts = select({
        "//config:x86_64-qnx": ["-lsecpol"],
        "//config:build_qnx8": ["-lsecpol"],
        "//conditions:default": ["-lpthread"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":config",
        "//externals/ipc_dropin",
        "//src/launch_manager_daemon/common",
        "//src/launch_manager_daemon/common:control_client_channel",
        "//src/launch_manager_daemon/common:identifier_hash",
        "//src/launch_manager_daemon/common:lifecycle_error",
        "//src/launch_manager_daemon/common:log",
        "//src/launch_manager_daemon/health_monitor_lib:health_monitor",
        "@flatbuffers",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/result",
    ],
)
