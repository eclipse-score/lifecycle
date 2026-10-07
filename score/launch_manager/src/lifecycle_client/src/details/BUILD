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
<<<<<<< HEAD
load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")
=======
load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")
>>>>>>> 26a8305d (test: Add report_running_impl_UT.cpp)

cc_library(
    name = "report_running_impl",
    srcs = ["report_running_impl.cpp"],
    hdrs = ["report_running_impl.hpp"],
    include_prefix = "score/mw/lifecycle/lifecycle_client/details",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src/details",
    visibility = [
        "//score/launch_manager/src/lifecycle_client:__pkg__",
        "//tests/utils/accessors:__pkg__",
    ],
    deps = [
        "//score/launch_manager:error_event",
        "//score/launch_manager/src/daemon/src/common:constants",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "mock_report_running_impl",
    testonly = True,
    srcs = ["mock_report_running_adapter.cpp"],
    hdrs = [
        "mock_report_running_impl.h",
        "report_running_impl.hpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle/lifecycle_client/details",
    strip_include_prefix = "/score/launch_manager/src/lifecycle_client/src/details",
    visibility = ["//score/launch_manager/src/lifecycle_client:__pkg__"],
    deps = [
        "//score/launch_manager:error_event",
        "//score/launch_manager/src/daemon/src/common:log",
        "//score/launch_manager/src/daemon/src/osal:ipc_comms",
        "@googletest//:gtest",
        "@score_baselibs//score/result",
    ],
)

lm_cc_test(
    name = "report_running_impl_UT",
    srcs = ["report_running_impl_UT.cpp"],
    copts = ["-DUNIT_TEST"],
    linkopts = [
        "-Wl,--wrap=fstat",
        "-Wl,--wrap=close",
        "-Wl,--wrap=getpid",
        "-Wl,--wrap=mmap",
        "-Wl,--wrap=fork",
        "-Wl,--wrap=execve",
        "-Wl,--wrap=kill",
        "-Wl,--wrap=wait",
        "-Wl,--wrap=shm_open",
        "-Wl,--wrap=shm_unlink",
        "-Wl,--wrap=ftruncate",
        "-Wl,--wrap=access",
        "-Wl,--wrap=munmap",
        "-Wl,--wrap=setpgid",
        "-Wl,--wrap=setgid",
        "-Wl,--wrap=setuid",
        "-Wl,--wrap=sched_setscheduler",
        "-Wl,--wrap=chdir",
        "-Wl,--wrap=setrlimit",
        "-Wl,--wrap=fcntl",
        "-Wl,--wrap=sem_init",
        "-Wl,--wrap=sem_destroy",
        "-Wl,--wrap=sem_trywait",
        "-Wl,--wrap=sem_post",
        "-Wl,--wrap=msync",
    ],
    linkstatic = True,
    deps = [
        ":report_running_impl",
        "//tests/utils/accessors:accessor_report_running_impl",
        "//tests/utils/mocks:mock_semaphore",
        "//tests/utils/mocks:mock_syscalls",
        "@googletest//:gtest_main",
    ],
)
