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
load("//tests/utils/bazel:integration.bzl", "integration_test")

cc_binary(
    name = "cpp_reporting_process",
    srcs = ["cpp_reporting_process.cpp"],
    deps = ["//score/launch_manager:lifecycle_cc"],
)

cc_binary(
    name = "c_reporting_process",
    srcs = ["c_reporting_process.c"],
    copts = ["-std=gnu11"],
    deps = ["//score/launch_manager:lifecycle_cc"],
)

rust_binary(
    name = "rust_reporting_process",
    srcs = ["rust_reporting_process.rs"],
    deps = ["//score/launch_manager:lifecycle_rust"],
)

integration_test(
    name = "lifecycle_lang_support",
    srcs = ["lifecycle_lang_support.py"],
    binaries = [
        ":c_reporting_process",
        ":cpp_reporting_process",
        ":rust_reporting_process",
        "//score/launch_manager",
        "//tests/utils/test_helper:fail_test",
        "//tests/utils/test_helper:verification_process",
    ],
    config = ":lifecycle_lang_support.json",
)
