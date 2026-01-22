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

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test", "rust_static_library")
load("@score_baselibs//:bazel/unit_tests.bzl", "cc_gtest_unit_test", "cc_unit_test_suites_for_host_and_qnx")

rust_library(
    name = "health_monitoring_lib",
    srcs = glob(["src/**/*.rs"]),
    crate_name = "health_monitoring_lib",
    proc_macro_deps = [
        "@score_baselibs_rust//src/testing_macros:score_testing_macros",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@score_baselibs_rust//src/log/score_log",
    ],
)


rust_static_library(
    name = "health_monitoring_lib_ffi",
    srcs = glob(["src/**/*.rs"]),
    crate_name = "health_monitoring_lib",
    proc_macro_deps = [
        "@score_baselibs_rust//src/testing_macros:score_testing_macros",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@score_baselibs_rust//src/log/score_log",
    ],
)

rust_test(
    name = "tests",
    crate = ":health_monitoring_lib",
    deps = [
        "@score_baselibs_rust//src/log/stdout_logger",
    ],
)

cc_library(
    name = "health_monitoring_lib_cc",
    srcs = [
        "cpp/deadline/deadline_monitor.cpp",
        "cpp/health_monitor.cpp",
        "cpp/common.cpp",
        "cpp/ffi_helpers.h",
        ],
    hdrs = [
        "cpp/include/score/hm/deadline/deadline_monitor.h",
        "cpp/include/score/hm/common.h",
        "cpp/include/score/hm/health_monitor.h"
    
    ],
    visibility = ["//visibility:public"],
    strip_include_prefix = "cpp/include",
    deps = [
        ":health_monitoring_lib_ffi",
        "@score_baselibs//score/result",
    ],
    copts = [
        "-Isrc/health_monitoring_lib/cpp",      # private include path
    ],
)

cc_gtest_unit_test(
    name = "cpp_tests",
    srcs = [
        "cpp/tests/health_monitor_test.cpp",
    ],
    deps = [
        ":health_monitoring_lib_cc",
    ],
)