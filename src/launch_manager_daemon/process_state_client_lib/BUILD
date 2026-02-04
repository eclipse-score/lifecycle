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
        "src/processstatenotifier.cpp",
        "src/processstatereceiver.cpp",
    ],
    hdrs = [
        "include/score/lcm/iprocessstatenotifier.hpp",
        "include/score/lcm/iprocessstatereceiver.hpp",
        "include/score/lcm/posixprocess.hpp",
        "include/score/lcm/processstatenotifier.hpp",
        "include/score/lcm/processstatereceiver.hpp",
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

cc_library(
    name = "headers",
    testonly = True,
    hdrs = glob(
        [
            "include/score/lcm/**/*.h",
            "include/score/lcm/**/*.hpp",
        ],
        allow_empty = True,  # "include/score/lcm/**/*.h" is empty as of now
    ),
    includes = [
        "include",
        "src",
    ],
    deps = [
        # Must match process_state_client deps
        "//externals/ipc_dropin",
        "//src/launch_manager_daemon/common",
        "//src/launch_manager_daemon/common:identifier_hash",
        "//src/launch_manager_daemon/common:lifecycle_error",
        "//src/launch_manager_daemon/common:log",
        "@score_baselibs//score/result",
    ],
)

cc_test(
    name = "processstateclient_UT",
    srcs = [
        "src/processstateclient_UT.cpp",
        "//src/launch_manager_daemon/process_state_client_lib:src/processstatenotifier.cpp",
        "//src/launch_manager_daemon/process_state_client_lib:src/processstatereceiver.cpp",
    ],
    deps = [
        "//src/launch_manager_daemon/process_state_client_lib:headers",
        "@googletest//:gtest_main",
    ],
)
