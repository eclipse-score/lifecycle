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

exports_files([
    "lm_flatcfg.fbs",
    "lm_flatcfg_generated.h",
])

filegroup(
    name = "lm_flatcfg_fbs",
    srcs = ["lm_flatcfg.fbs"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "lm_flatcfg",
    hdrs = ["lm_flatcfg_generated.h"],
    include_prefix = "score/mw/launch_manager/configuration",
    strip_include_prefix = "/score/launch_manager/daemon/src/configuration",
    visibility = ["//score:__subpackages__"],
    deps = ["@flatbuffers"],
)

cc_library(
    name = "configuration_manager",
    srcs = ["configuration_manager.cpp"],
    hdrs = ["configuration_manager.hpp"],
    include_prefix = "score/mw/launch_manager/configuration",
    strip_include_prefix = "/score/launch_manager/daemon/src/configuration",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":lm_flatcfg",
        "//score/launch_manager:error",
        "//score/launch_manager/daemon/src/common:constants",
        "//score/launch_manager/daemon/src/common:identifier_hash",
        "//score/launch_manager/daemon/src/common:log",
        "//score/launch_manager/daemon/src/common:process_group_state_id",
        "//score/launch_manager/daemon/src/osal:num_cores",
        "//score/launch_manager/daemon/src/process_group_manager:iprocess",
        "//score/launch_manager/daemon/src/process_state_client:posix_process",
        "@flatbuffers",
    ],
)
