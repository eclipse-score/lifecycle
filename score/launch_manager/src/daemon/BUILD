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
load("@rules_cc//cc:defs.bzl", "cc_binary")
load(":daemon.bzl", "debug_asserts_binary")

cc_library(
    name = "run",
    srcs = ["src/run.cpp"],
    hdrs = ["src/run.hpp"],
    include_prefix = "score/mw/launch_manager",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/",
    visibility = ["//score/launch_manager/src/daemon:__pkg__"],
    deps = [
        "//score/launch_manager/src/daemon/src/alive_monitor",
        "//score/launch_manager/src/daemon/src/common:assertion_handler",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/configuration:flatbuffer_config_loader",
        "//score/launch_manager/src/daemon/src/control:control_provider",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "//score/launch_manager/src/daemon/src/process_group_manager",
        "//score/launch_manager/src/daemon/src/recovery_client",
        "//score/launch_manager/src/daemon/src/watchdog:watchdog_factory",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_binary(
    name = "launch_manager",
    srcs = ["src/main.cpp"],
    linkopts = select({
        # glibc v2.34 integrated libpthread, libdl, libutil, libanl and librt into libc.
        # However we keep these links to the old libraries for compatibility with older glibc versions.
        "@platforms//os:linux": [
            "-lpthread",
            "-lrt",
        ],
        "//conditions:default": [],
    }),
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon:run",
    ],
)

# Integration-test variant of :launch_manager - the same binary rebuilt with
# SCORE_LANGUAGE_FUTURECPP_ASSERT_LEVEL_DEBUG applied to every translation
# unit (see daemon.bzl), so the integration suite exercises DBG-level asserts
# while the production :launch_manager target stays untouched (lifecycle#843;
# lifecycle#579 did the same for unit tests via lm_cc_test).
debug_asserts_binary(
    name = "launch_manager_debug_asserts",
    binary = ":launch_manager",
    out = "debug_asserts/launch_manager",
    visibility = ["//score/launch_manager:__subpackages__"],
)
