# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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
cc_library(
    name = "process_state_client",
    srcs = [
        "src/processstateclient.cpp",
    ],
    hdrs = [
        "include/score/lcm/posixprocess.hpp",
        "include/score/lcm/processstateclient.hpp",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//externals/ipc_dropin",
        "//src/launch_manager_daemon/common",
        "//src/launch_manager_daemon/common:identifier_hash",
        "//src/launch_manager_daemon/common:lifecycle_error",
        "//src/launch_manager_daemon/common:log",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/result",
    ],
)
