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

load(":wrapper.bzl", "expand_executable_template")

expand_executable_template(
    name = "tsan_setarch_wrapper_sh",
    out = "tsan_setarch_wrapper.sh",
    template = "tsan_setarch_wrapper.sh.tpl",
    wrapper = "@score_cpp_policies//sanitizers:wrapper",
)

sh_binary(
    name = "tsan_setarch_wrapper",
    srcs = [":tsan_setarch_wrapper_sh"],
    data = [
        "@bazel_tools//tools/bash/runfiles",
        "@score_cpp_policies//sanitizers:wrapper",
    ],
    target_compatible_with = ["@platforms//os:linux"],
    visibility = ["//score:__subpackages__"],
)
