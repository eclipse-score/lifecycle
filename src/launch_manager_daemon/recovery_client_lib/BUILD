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
    name = "recovery_client",
    srcs = [
        "src/recovery_client.cpp",
    ],
    hdrs = [
        "include/score/lcm/irecovery_client.h",
        "include/score/lcm/recovery_client.hpp",
    ],
    includes = ["include"],
    visibility = [
        "//src/health_monitor_lib:__subpackages__",
        "//src/launch_manager_daemon:__subpackages__",
    ],
    deps = [
        "//externals/ipc_dropin",
        "//src/launch_manager_daemon/common:identifier_hash",
        "//src/launch_manager_daemon/common:lifecycle_error",
        "@score_baselibs//score/concurrency/future",
    ],
)
