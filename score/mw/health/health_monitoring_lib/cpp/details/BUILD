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

cc_library(
    name = "log_init",
    srcs = ["log_init.cpp"],
    visibility = ["//score/mw/health/health_monitoring_lib/cpp:__pkg__"],
    deps = [
        "@googletest//:gtest",
        "@score_baselibs_rust//src/log/stdout_logger_cpp_init",
    ],
)

cc_library(
    name = "thread",
    srcs = ["thread.cpp"],
    hdrs = ["thread.h"],
    include_prefix = "score/mw/health/health_monitoring_lib",
    strip_include_prefix = "/score/mw/health/health_monitoring_lib/cpp/details",
    visibility = ["//score/mw/health/health_monitoring_lib/cpp:__pkg__"],
    deps = [
        "//score/mw/health/health_monitoring_lib/cpp:common",
        "@score_baselibs//score/language/futurecpp",
    ],
)
