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
    name = "report_running_impl",
    srcs = ["report_running_impl.cpp"],
    hdrs = ["report_running_impl.hpp"],
    include_prefix = "score/mw/lifecycle/lifecycle_client/details",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src/details",
    visibility = ["//score/launch_manager/lifecycle_client:__pkg__"],
    deps = [
        "//score/launch_manager:error_event",
        "//score/launch_manager/daemon/src/common:constants",
        "//score/launch_manager/daemon/src/common:log",
        "//score/launch_manager/daemon/src/osal:ipc_comms",
        "@score_baselibs//score/result",
    ],
)
