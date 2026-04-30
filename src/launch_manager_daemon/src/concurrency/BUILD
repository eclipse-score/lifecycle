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
load("@rules_cc//cc:defs.bzl", "cc_test")
load("//config:common_cc.bzl", "cc_library_with_common_opts")

cc_library(
    name = "helgrind_annotations",
    hdrs = ["helgrind_annotations.hpp"],
    strip_include_prefix = "//src/launch_manager_daemon/src",
    visibility = ["//src:__subpackages__"],
)

cc_library(
    name = "mpmc_concurrent_queue",
    hdrs = [
        "concurrency_error_domain.hpp",
        "mpmc_concurrent_queue.hpp",
    ],
    linkopts = [
        "-l:libatomic.a",
    ],
    strip_include_prefix = "//src/launch_manager_daemon/src",
    visibility = ["//src:__subpackages__"],
    deps = [
        ":helgrind_annotations",
        "//src/launch_manager_daemon/common:osal",
        "@score_baselibs//score/result",
    ],
)

cc_test(
    name = "concurrency_error_domain_test",
    srcs = ["concurrency_error_domain_test.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":mpmc_concurrent_queue",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mpmc_concurrent_queue_test",
    srcs = ["mpmc_concurrent_queue_test.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":mpmc_concurrent_queue",
        "@googletest//:gtest_main",
    ],
)

# Run with:
# bazel test --run_under="valgrind --tool=helgrind --suppressions=src/launch_manager_daemon/src/concurrency/helgrind.supp --error-exitcode=1" //src/launch_manager_daemon/src/concurrency:mpmc_concurrent_queue_helgrind_test --config=host --test_output=all
# Note: This is using your host packages so you need to install valgrind.
# Two tests intentionally exercise semaphore failure paths (EINTR / EAGAIN).
# Those expected PthAPIerror reports are silenced via helgrind.supp.
cc_test(
    name = "mpmc_concurrent_queue_helgrind_test",
    srcs = ["mpmc_concurrent_queue_test.cpp"],
    data = ["helgrind.supp"],
    tags = [
        "helgrind",
        "manual",
    ],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":mpmc_concurrent_queue",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mpmc_concurrent_queue_tsan_test",
    srcs = ["mpmc_concurrent_queue_test.cpp"],
    copts = [
        "-fsanitize=thread",
        "-O0",
        "-g",
    ],
    linkopts = ["-fsanitize=thread"],
    tags = [
        "no-coverage",  # coverage + tsan might cause problems
        "tsan",
    ],
    visibility = ["//tests:__subpackages__"],
    deps = [
        ":mpmc_concurrent_queue",
        "@googletest//:gtest_main",
    ],
)
