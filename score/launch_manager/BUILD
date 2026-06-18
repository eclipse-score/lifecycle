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
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "error_event",
    hdrs = [
        "execution_error.h",
        "execution_error_event.h",
    ],
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager",
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        ":error",
        "//score/launch_manager/daemon/src/common:identifier_hash",
    ],
)

cc_test(
    name = "exec_error_domain_UT",
    srcs = ["exec_error_domain_UT.cpp"],
    visibility = ["//visibility:private"],
    deps = [
        ":error",
        "@googletest//:gtest_main",
        "@score_baselibs//score/result",
    ],
)

alias(
    name = "applicationcontext_mock_cc",
    actual = "//score/launch_manager/lifecycle_client:applicationcontext_mock",
)

alias(
    name = "lifecycle_mock_cc",
    actual = "//score/launch_manager/lifecycle_client:lifecycle_mock",
)

alias(
    name = "report_running_mock_cc",
    actual = "//score/launch_manager/lifecycle_client:report_running_mock",
)

# BEGIN: Fine granular targets only needed for testing purposes
alias(
    name = "aasapplicationcontainer_cc",
    actual = "//score/launch_manager/lifecycle_client:aasapplicationcontainer",
)

alias(
    name = "application_cc",
    actual = "//score/launch_manager/lifecycle_client:application",
)

alias(
    name = "applicationcontext_cc",
    actual = "//score/launch_manager/lifecycle_client:applicationcontext",
)

alias(
    name = "lifecyclemanager_cc",
    actual = "//score/launch_manager/lifecycle_client:lifecyclemanager",
)

alias(
    name = "report_running_cc",
    actual = "//score/launch_manager/lifecycle_client:report_running",
)

alias(
    name = "runapplication_cc",
    actual = "//score/launch_manager/lifecycle_client:runapplication",
)
# END: Fine granular targets only needed for testing purposes
