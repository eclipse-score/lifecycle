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
load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "cpp_supervised_app",
    srcs = [
        "main.cpp",
    ],
    copts = select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": [],
    }),
    linkopts = select({
        "@platforms//os:qnx": [
            "-lsocket",
        ],
        "@platforms//os:linux": [
            "-lpthread",
            "-lrt",
        ],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "//score/health_monitor:health_monitoring_cc",
        "//score/launch_manager:alive_cc",
        "//score/launch_manager:lifecycle_cc",
        "@score_baselibs_rust//src/log/stdout_logger_cpp_init",
    ],
)
