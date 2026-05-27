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
    name = "helgrind_annotations",
    hdrs = ["helgrind_annotations.hpp"],
    include_prefix = "score/mw/launch_manager/common/concurrency/details",
    strip_include_prefix = "/score/launch_manager/daemon/src/common/concurrency/details",
    visibility = ["//score:__subpackages__"],
)
