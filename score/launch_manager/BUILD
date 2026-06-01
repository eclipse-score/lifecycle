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

package(default_visibility = ["//visibility:public"])

alias(
    name = "control_cc",
    actual = "//score/launch_manager/control_client:control_client",
)

alias(
    name = "alive_cc",
    actual = "//score/launch_manager/daemon/src/alive_monitor:am_shared_lib",
)

alias(
    name = "alive_rust",
    actual = "//score/launch_manager/daemon/src/alive_monitor/rust:monitor_rs",
)

alias(
    name = "lifecycle_cc",
    actual = "//score/launch_manager/lifecycle_client:lifecycle_client",
)

alias(
    name = "lifecycle_rust",
    actual = "//score/launch_manager/lifecycle_client/src/rust:lifecycle_client_rs",
)

alias(
    name = "launch_manager",
    actual = "//score/launch_manager/daemon:launch_manager",
)

cc_library(
    name = "error",
    hdrs = ["execution_error.h"],
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager",
)

cc_library(
    name = "error_event",
    hdrs = [
        "execution_error.h",
        "execution_error_event.h",
    ],
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager",
    deps = [
        ":error",
        "//score/launch_manager/daemon/src/common:identifier_hash",
    ],
)
