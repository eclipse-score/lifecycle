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
        "link-arg=-lstdc++",
        "-C",
        "link-arg=-lc",
    ],
    deps = [
        "//src/launch_manager_daemon/health_monitor_lib/rust_bindings:monitor_rs",
        "//src/launch_manager_daemon/lifecycle_client_lib/rust_bindings:lifecycle_client_rs",
        "@score_crates//:clap",
        "@score_crates//:libc",
        "@score_crates//:signal_hook",
    ],
)
