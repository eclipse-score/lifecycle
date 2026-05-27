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
load("@rules_rust//rust:defs.bzl", "rust_binary")

package(default_visibility = ["//visibility:public"])

rust_binary(
    name = "rust_supervised_app",
    srcs = [
        "src/main.rs",
    ],
    rustc_flags = [
        "-C",
        "link-arg=-lm",
        "-C",
        "link-arg=-lc",
    ] + select({
        "@platforms//os:qnx": [
            "-Clink-arg=-lc++",
        ],
        "@platforms//os:linux": [
            "-Clink-arg=-lrt",
            "-Clink-arg=-lstdc++",
        ],
    }),
    deps = [
        "//score/health_monitor:health_monitoring_rust",
        "//score/launch_manager:lifecycle_rust",
        "@score_baselibs_rust//src/log/score_log",
        "@score_baselibs_rust//src/log/stdout_logger",
        "@score_crates//:clap",
        "@score_crates//:libc",
        "@score_crates//:signal_hook",
    ],
)
