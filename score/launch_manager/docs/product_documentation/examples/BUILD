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
load("@bazel_skylib//rules:diff_test.bzl", "diff_test")
load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_rust//rust:defs.bzl", "rust_binary")

# note this is only used for docs
# It shall be built to verify the docs are working
cc_binary(
    name = "example_stm",
    srcs = [
        "example_state_manager.cpp",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "@score_baselibs//score/string_manipulation/arguments",
        "@score_communication//score/mw/com",
    ],
)

cc_binary(
    name = "example_c",
    srcs = [
        "c_reporting_process.c",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//score/launch_manager:lifecycle_cc",
    ],
)

cc_binary(
    name = "example_cpp",
    srcs = [
        "cpp_reporting_process.cpp",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//score/launch_manager:lifecycle_cc",
    ],
)

cc_binary(
    name = "example_cpp_lifecycle_app",
    srcs = [
        "cpp_lifecycle_application.cpp",
    ],
    copts = select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": ["-pthread"],
    }),
    linkopts = select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": ["-lpthread"],
    }),
    visibility = ["//visibility:private"],
    deps = [
        "//score/launch_manager:lifecycle_cc",
    ],
)

rust_binary(
    name = "example_rust",
    srcs = [
        "rust_reporting_process.rs",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//score/launch_manager:lifecycle_rust",
    ],
)
