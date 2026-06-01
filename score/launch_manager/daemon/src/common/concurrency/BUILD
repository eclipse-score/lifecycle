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
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "workerthread",
    hdrs = ["workerthread.hpp"],
    include_prefix = "score/mw/launch_manager/common/concurrency",
    strip_include_prefix = "/score/launch_manager/daemon/src/common/concurrency",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":mpmc_concurrent_queue",
        "//score/launch_manager/daemon/src/common:constants",
        "//score/launch_manager/daemon/src/common:log",
    ],
)

cc_library(
    name = "mpmc_concurrent_queue",
    hdrs = [
        "concurrency_error_domain.hpp",
        "mpmc_concurrent_queue.hpp",
    ],
    include_prefix = "score/mw/launch_manager/common/concurrency",
    linkopts = [
        "-l:libatomic.a",
    ],
    strip_include_prefix = "/score/launch_manager/daemon/src/common/concurrency",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager/daemon/src/common/concurrency/details:helgrind_annotations",
        "//score/launch_manager/daemon/src/osal:return_types",
        "//score/launch_manager/daemon/src/osal:semaphore",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/mw/log",
    ],
)

cc_test(
    name = "mpmc_concurrent_queue_test",
    srcs = ["mpmc_concurrent_queue_test.cpp"],
    deps = [
        ":mpmc_concurrent_queue",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mpmc_concurrent_queue_helgrind_test",
    srcs = ["mpmc_concurrent_queue_test.cpp"],
    data = ["mpmc_concurrent_queue_test_helgrind.supp"],
    tags = [
        "helgrind",
        "manual",
    ],
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
        "no-coverage",
        "tsan",
    ],
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        ":mpmc_concurrent_queue",
        "@googletest//:gtest_main",
    ],
)
