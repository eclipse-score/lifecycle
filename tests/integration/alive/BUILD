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
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_rust//rust:defs.bzl", "rust_binary")

package(default_visibility = ["//tests/integration/alive:__subpackages__"])

cc_binary(
    name = "cpp_alive_process",
    srcs = ["cpp_alive_process.cpp"],
    deps = [
        "//tests/utils/test_helper:arg_parser",
        "//score/launch_manager:alive_cc",
        "//score/launch_manager:lifecycle_cc",
    ],
)

rust_binary(
    name = "rust_alive_process",
    srcs = ["rust_alive_process.rs"],
    target_compatible_with = [
        "@score_cpp_policies//sanitizers/constraints:no_tsan",
        "@score_cpp_policies//sanitizers/constraints:no_asan",
    ],
    deps = [
        "//score/launch_manager:alive_rust",
        "//score/launch_manager:lifecycle_rust",
    ],
)
