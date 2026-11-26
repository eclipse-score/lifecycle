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

# IWYU mapping files, used via --@bazel_iwyu//:iwyu_mappings (see iwyu.bazelrc).
filegroup(
    name = "mappings",
    srcs = [
        "score_baselibs.imp",
        # Keep the mappings that bazel_iwyu provides by default (gtest).
        "@bazel_iwyu//:iwyu_mappings_default",
    ],
    visibility = ["//visibility:public"],
)
