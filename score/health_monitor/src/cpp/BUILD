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
load("//tests/utils/bazel:unit_test.bzl", "cc_gtest_unit_test")

# The package root is the include root: headers are included as "score/health_monitor/<name>.h".
cc_library(
    name = "health_monitor",
    srcs = [
        "deadline.cpp",
        "error.cpp",
    ],
    hdrs = [
        "deadline.h",
        "deadline_monitor.h",
        "deadline_monitor_builder.h",
        "error.h",
        "health_monitor.h",
        "health_monitor_builder.h",
        "heartbeat_monitor.h",
        "heartbeat_monitor_builder.h",
        "logic_monitor.h",
        "logic_monitor_builder.h",
        "tag.h",
        "taken.h",
        "thread_parameters.h",
        "time_range.h",
    ],
    include_prefix = "score/health_monitor",
    strip_include_prefix = "",
    visibility = ["//visibility:public"],
    deps = [
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
        "@score_baselibs//score/result:error",
    ],
)

# gmock classes for tests of application code that takes the health monitor interfaces.
cc_library(
    name = "mock",
    testonly = True,
    hdrs = [
        "mock_deadline.h",
        "mock_deadline_monitor.h",
        "mock_health_monitor.h",
        "mock_heartbeat_monitor.h",
        "mock_logic_monitor.h",
        "mock_taken.h",
    ],
    include_prefix = "score/health_monitor",
    strip_include_prefix = "",
    visibility = ["//visibility:public"],
    deps = [
        ":health_monitor",
        "@googletest//:gtest",
    ],
)

cc_gtest_unit_test(
    name = "tests",
    srcs = [
        "deadline_test.cpp",
        "error_test.cpp",
        "mock_usage_test.cpp",
        "tag_test.cpp",
        "taken_test.cpp",
        "time_range_test.cpp",
    ],
    target_compatible_with = select({
        "//config:unit_qemu": [],
        "//config:unit_host": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [
        ":health_monitor",
        ":mock",
    ],
)
