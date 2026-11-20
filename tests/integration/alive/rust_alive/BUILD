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
load("@rules_rust//rust:defs.bzl", "rust_binary")
load("//tests/utils/bazel:integration.bzl", "integration_test")

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

integration_test(
    name = "rust_alive",
    srcs = ["rust_alive.py"],
    binaries = [
        ":rust_alive_process",
        "//score/launch_manager",
        "//tests/integration/alive:control_client_test_driver",
        "//tests/utils/test_helper:verification_process",
    ],
    config = ":rust_alive.json",
)
