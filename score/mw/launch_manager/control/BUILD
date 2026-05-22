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
    name = "control_client_channel",
    srcs = [
        "control_client_channel.cpp",
    ],
    hdrs = [
        "control_client_channel.hpp",
    ],
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/mw/launch_manager/common:constants",
        "//score/mw/launch_manager/common:log",
        "//score/mw/launch_manager/common:process_group_state_id",
        "//score/mw/launch_manager/osal:ipc_comms",
    ],
)
