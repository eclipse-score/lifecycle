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
    visibility = ["//score:__subpackages__"],
    deps = ["@flatbuffers"],
)

cc_library(
    name = "configuration_manager",
    srcs = ["configuration_manager.cpp"],
    hdrs = ["configuration_manager.hpp"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":lm_flatcfg",
        "//score/mw/launch_manager/common:constants",
        "//score/mw/launch_manager/common:identifier_hash",
        "//score/mw/launch_manager/common:log",
        "//score/mw/launch_manager/common:process_group_state_id",
        "//score/mw/launch_manager/osal:num_cores",
        "//score/mw/launch_manager/process_group_manager:iprocess",
        "//score/mw/launch_manager/process_state_client:posix_process",
        "//score/mw/lifecycle:error",
        "@flatbuffers",
    ],
)
