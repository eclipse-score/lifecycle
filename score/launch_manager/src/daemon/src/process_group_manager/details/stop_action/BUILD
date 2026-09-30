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

cc_library(
    name = "istop_action",
    hdrs = ["istop_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/process_group_manager/details:handle",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "mock_stop_action",
    testonly = True,
    hdrs = ["mock_stop_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":istop_action",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "empty_stop_action",
    srcs = ["empty_stop_action.cpp"],
    hdrs = ["empty_stop_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/stop_action",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/stop_action",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [":istop_action"],
)

cc_library(
    name = "process_stop_action",
    srcs = ["process_stop_action.cpp"],
    hdrs = ["process_stop_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/stop_action",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/stop_action",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":istop_action",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:process_launcher",
    ],
)
