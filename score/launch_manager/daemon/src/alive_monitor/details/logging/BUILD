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
    name = "phm_logging",
    srcs = ["PhmLogger.cpp"],
    hdrs = ["PhmLogger.hpp"],
    defines = select({
        "//config:lm_use_cout_log": [],
        "//conditions:default": ["LC_LOG_SCORE_MW_LOG"],
    }),
    include_prefix = "score/mw/launch_manager/alive_monitor/details/logging",
    strip_include_prefix = "/score/launch_manager/daemon/src/alive_monitor/details/logging",
    visibility = ["//score/launch_manager/daemon/src/alive_monitor:__subpackages__"],
    deps = ["@score_baselibs//score/mw/log"],
)
