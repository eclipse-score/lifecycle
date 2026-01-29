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
cc_binary(
    name = "cpp_supervised_app",
    srcs = [
        "main.cpp",
    ],
    copts = select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": ["-pthread"],
    }),
    linkopts = select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": [
            "-lpthread",
            "-lrt",
        ],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "//src/health_monitoring_lib:health_monitoring_lib_cc",
        "//src/launch_manager_daemon/health_monitor_lib:hm_shared_lib",
        "//src/launch_manager_daemon/lifecycle_client_lib:lifecycle_client",
        "@score_baselibs_rust//src/log/stdout_logger_cpp_init",
    ],
)
