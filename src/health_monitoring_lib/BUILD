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

load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_rust//rust:defs.bzl", "rust_library", "rust_static_library", "rust_test")
load("@score_baselibs//:bazel/unit_tests.bzl", "cc_gtest_unit_test")

COMMON_DEPS = [
    "@score_baselibs_rust//src/containers",
    "@score_baselibs_rust//src/log/score_log",
    "//src/launch_manager_daemon/health_monitor_lib/rust_bindings:monitor_rs",
]

PROC_MACRO_DEPS = [
    "@score_baselibs_rust//src/testing_macros:score_testing_macros",
]

CC_SOURCES = [
    "cpp/common.cpp",
    "cpp/deadline_monitor.cpp",
    "cpp/ffi_helpers.h",
    "cpp/health_monitor.cpp",
]

CC_HDRS = [
    "cpp/include/score/hm/common.h",
    "cpp/include/score/hm/deadline/deadline_monitor.h",
    "cpp/include/score/hm/health_monitor.h",
]

# Health Monitoring Library - Rust
rust_library(
    name = "health_monitoring_lib",
    srcs = glob(["rust/**/*.rs"]),
    proc_macro_deps = PROC_MACRO_DEPS,
    visibility = ["//visibility:public"],
    deps = COMMON_DEPS,
)

# Health Monitoring Library - C++ Bindings
cc_library(
    name = "health_monitoring_lib_cc",
    srcs = CC_SOURCES,
    hdrs = CC_HDRS,
    strip_include_prefix = "cpp/include",
    visibility = ["//visibility:public"],
    deps = [
        ":health_monitoring_lib_ffi",
        "@score_baselibs//score/result",
    ],
)

# Health Monitoring Library - Rust FFI
rust_static_library(
    name = "health_monitoring_lib_ffi",
    srcs = glob(["rust/**/*.rs"]),
    crate_name = "health_monitoring_lib",
    proc_macro_deps = [
        "@score_baselibs_rust//src/testing_macros:score_testing_macros",
    ],
    visibility = ["//visibility:private"],
    deps = COMMON_DEPS,
)

# Health Monitoring Library - Rust Stub Supervisor API Client
rust_static_library(
    name = "health_monitoring_lib_stub_supervisor",
    srcs = glob(["rust/**/*.rs"]),
    crate_features = ["stub_supervisor_api_client"],  # Uses the stub supervisor API client, used for C++ testing
    proc_macro_deps = PROC_MACRO_DEPS,
    visibility = ["//visibility:private"],
    deps = COMMON_DEPS,
)

cc_library(
    name = "health_monitoring_lib_cc_stub_supervisor",
    srcs = CC_SOURCES,
    hdrs = CC_HDRS,
    strip_include_prefix = "cpp/include",
    visibility = ["//visibility:private"],
    deps = [
        ":health_monitoring_lib_stub_supervisor",
        "@score_baselibs//score/result",
    ],
)

rust_test(
    name = "tests",
    crate = ":health_monitoring_lib",
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
        "@score_baselibs_rust//src/log/stdout_logger",
    ],
)

cc_gtest_unit_test(
    name = "cpp_tests",
    srcs = [
        "cpp/tests/health_monitor_test.cpp",
        "cpp/tests/log_init.cpp",
    ],
    linkopts = select({
        "@platforms//os:qnx": ["-lsocket"],
        "@platforms//os:linux": [],
    }),
    deps = [
        ":health_monitoring_lib_cc_stub_supervisor",
        "@score_baselibs_rust//src/log/stdout_logger_cpp_init",
    ],
)
