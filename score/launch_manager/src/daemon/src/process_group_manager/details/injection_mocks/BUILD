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
    name = "mock_graph",
    testonly = True,
    hdrs = ["graph.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/injection_mocks",
    visibility = [
        "//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__",
    ],
    deps = [
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/common/concurrency:concurrency_error_domain",
        "//score/launch_manager/src/daemon/src/common/concurrency:mpmc_concurrent_queue",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "//score/launch_manager/src/daemon/src/configuration:config",
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "//score/launch_manager/src/daemon/src/osal:semaphore",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
        "//score/launch_manager/src/daemon/src/process_group_manager:irun_target_control",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:component_event",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:component_of",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:component_task",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:dependency_graph",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:icomponent",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:process_handling",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:process_info_node",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:run_target",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:transition",
        "//score/launch_manager/src/lm_control",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
        "@score_baselibs//score/result:error",
    ],
)

cc_library(
    name = "mock_process_monitor",
    testonly = True,
    hdrs = ["process_monitor.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/injection_mocks",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/common:identifier_hash",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:component_event",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:component_task",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:icomponent",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:icomponent_controller",
        "//score/launch_manager/src/daemon/src/process_group_manager/details:icomponent_event_publisher_consumer",
        "@score_baselibs//score/language/futurecpp",
    ],
)
