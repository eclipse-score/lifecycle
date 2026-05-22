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
    name = "health_monitoring_lib_cc",
    visibility = ["//visibility:public"],
    deps = [
        "//score/mw/health/health_monitoring_lib/cpp:health_monitor",
        "//score/mw/health/health_monitoring_lib/rust:health_monitoring_lib_ffi",
    ],
)

cc_library(
    name = "health_monitoring_lib_cc_stub_supervisor",
    visibility = ["//score/mw/health/health_monitoring_lib/cpp:__pkg__"],
    deps = [
        "//score/mw/health/health_monitoring_lib/cpp:health_monitor",
        "//score/mw/health/health_monitoring_lib/rust:health_monitoring_lib_stub_supervisor",
    ],
)
