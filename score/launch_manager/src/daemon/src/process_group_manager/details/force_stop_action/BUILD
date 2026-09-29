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
    name = "iforce_stop_action",
    hdrs = ["iforce_stop_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/process_group_manager/details:handle",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "mock_force_stop_action",
    testonly = True,
    hdrs = ["mock_force_stop_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":iforce_stop_action",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "empty_force_stop_action",
    srcs = ["empty_force_stop_action.cpp"],
    hdrs = ["empty_force_stop_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/force_stop_action",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/force_stop_action",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [":iforce_stop_action"],
)
