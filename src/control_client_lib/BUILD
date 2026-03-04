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
    name = "control_client_lib",
    srcs = [
        "src/control_client.cpp",
        "src/control_client_impl.cpp",
        "src/control_client_impl.hpp",
    ],
    hdrs = [
        "include/score/lcm/control_client.h",
        "include/score/lcm/execution_error_event.h",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        "//src/launch_manager_daemon/common",
        "//src/launch_manager_daemon/common:control_client_channel",
        "//src/launch_manager_daemon/common:identifier_hash",
        "//src/launch_manager_daemon/common:lifecycle_error",
        "//src/launch_manager_daemon/common:log",
        "//src/launch_manager_daemon/common:osal",
        "@score_baselibs//score/concurrency/future",
        "@score_baselibs//score/mw/log",
    ],
)
