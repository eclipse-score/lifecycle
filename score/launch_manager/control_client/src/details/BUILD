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
    name = "control_client_impl",
    srcs = [
        "control_client_impl.cpp",
    ],
    hdrs = [
        "control_client_impl.hpp",
    ],
    include_prefix = "score/mw/lifecycle/control_client/details",
    strip_include_prefix = "/score/launch_manager/control_client/src/details",
    visibility = ["//score/launch_manager/control_client:__subpackages__"],
    deps = [
        "//score/launch_manager:error_event",
        "//score/launch_manager/daemon/src/common:constants",
        "//score/launch_manager/daemon/src/common:identifier_hash",
        "//score/launch_manager/daemon/src/common:log",
        "//score/launch_manager/daemon/src/control:control_client_channel",
        "//score/launch_manager/daemon/src/osal:semaphore",
        "@score_baselibs//score/concurrency/future",
    ],
)
