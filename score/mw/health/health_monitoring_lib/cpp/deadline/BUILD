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
    name = "deadline_monitor",
    srcs = ["deadline_monitor.cpp"],
    hdrs = ["deadline_monitor.h"],
    include_prefix = "score/mw/health/health_monitoring_lib",
    strip_include_prefix = "/score/mw/health/health_monitoring_lib/cpp",
    visibility = ["//visibility:public"],
    deps = [
        "//score/mw/health/health_monitoring_lib/cpp:common",
        "//score/mw/health/health_monitoring_lib/cpp:tag",
        "@score_baselibs//score/result",
    ],
)
