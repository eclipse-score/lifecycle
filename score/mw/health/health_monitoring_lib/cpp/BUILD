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

load("@rules_cc//cc:defs.bzl", "cc_library")
load("@score_baselibs//:bazel/unit_tests.bzl", "cc_gtest_unit_test")

cc_library(
    name = "tag",
    hdrs = ["tag.h"],
    include_prefix = "score/mw/health/health_monitoring_lib",
    strip_include_prefix = "/score/mw/health/health_monitoring_lib/cpp",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "common",
    srcs = ["common.cpp"],
    hdrs = ["common.h"],
    include_prefix = "score/mw/health/health_monitoring_lib",
    strip_include_prefix = "/score/mw/health/health_monitoring_lib/cpp",
    visibility = ["//visibility:public"],
    deps = ["@score_baselibs//score/language/futurecpp"],
)

cc_library(
    name = "health_monitor",
    srcs = ["health_monitor.cpp"],
    hdrs = ["health_monitor.h"],
    include_prefix = "score/mw/health/health_monitoring_lib",
    strip_include_prefix = "/score/mw/health/health_monitoring_lib/cpp",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":common",
        ":tag",
        "//score/mw/health/health_monitoring_lib/cpp/deadline:deadline_monitor",
        "//score/mw/health/health_monitoring_lib/cpp/details:thread",
        "//score/mw/health/health_monitoring_lib/cpp/heartbeat:heartbeat_monitor",
        "//score/mw/health/health_monitoring_lib/cpp/logic:logic_monitor",
    ],
)

cc_gtest_unit_test(
    name = "cpp_tests",
    srcs = [
        "health_monitor_test.cpp",
    ],
    linkopts = select({
        "@platforms//os:qnx": ["-lsocket"],
        "@platforms//os:linux": [],
    }),
    deps = [
        "//score/mw/health/health_monitoring_lib:health_monitoring_lib_cc_stub_supervisor",
        "//score/mw/health/health_monitoring_lib/cpp/details:log_init",
    ],
)
