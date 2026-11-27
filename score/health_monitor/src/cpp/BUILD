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
load("//tests/utils/bazel:unit_test.bzl", "cc_gtest_unit_test")

# The package root is the include root: headers are included as "score/health_monitor/<name>.h".
cc_library(
    name = "error",
    srcs = ["error.cpp"],
    hdrs = ["error.h"],
    include_prefix = "score/health_monitor",
    strip_include_prefix = "",
    visibility = ["//visibility:public"],
    deps = [
        "@score_baselibs//score/result",
        "@score_baselibs//score/result:error",
    ],
)

# Interface that every supervisor client backend implements, together with SupervisorClient::Create().
cc_library(
    name = "supervisor_client_interface",
    hdrs = ["supervisor_client.h"],
    include_prefix = "score/health_monitor",
    strip_include_prefix = "",
    visibility = ["//visibility:public"],
    deps = [
        ":error",
        "@score_baselibs//score/result",
    ],
)

# Everything except a supervisor client backend.
cc_library(
    name = "health_monitor_base",
    srcs = [
        "deadline.cpp",
        "health_monitor_builder.cpp",
        "heartbeat_monitor/heartbeat_monitor_impl.cpp",
        "heartbeat_monitor/heartbeat_monitor_impl.h",
        "heartbeat_monitor/heartbeat_state.h",
        "internal/fetch_update.h",
        "internal/health_monitor_impl.cpp",
        "internal/health_monitor_impl.h",
        "internal/logger.h",
        "internal/monitor_evaluator.h",
        "internal/monitoring_logic.cpp",
        "internal/monitoring_logic.h",
        "internal/monotonic_time.h",
        "internal/thread_setup.cpp",
        "internal/thread_setup.h",
        "internal/worker.cpp",
        "internal/worker.h",
        "thread_parameters.cpp",
    ],
    hdrs = [
        "deadline.h",
        "deadline_monitor.h",
        "deadline_monitor_builder.h",
        "health_monitor.h",
        "health_monitor_builder.h",
        "heartbeat_monitor.h",
        "heartbeat_monitor_builder.h",
        "logic_monitor.h",
        "logic_monitor_builder.h",
        "tag.h",
        "taken.h",
        "thread_parameters.h",
        "time_range.h",
    ],
    include_prefix = "score/health_monitor",
    strip_include_prefix = "",
    deps = [
        ":error",
        ":supervisor_client_interface",
        "@score_baselibs//score/concurrency:interruptible_wait",
        "@score_baselibs//score/containers:non_relocatable_vector",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/os:time",
        "@score_baselibs//score/result",
    ],
)

# Supervisor client over the alive API of the launch manager.
# The process needs the IDENTIFIER environment variable.
cc_library(
    name = "alive_supervisor_client",
    srcs = ["supervisor_client/alive_supervisor_client.cpp"],
    hdrs = ["supervisor_client/alive_supervisor_client.h"],
    deps = [
        ":health_monitor_base",
        "//score/launch_manager:alive_cc",
    ],
    alwayslink = True,
)

# Supervisor client that needs no supervisor: it connects at once and only logs each alive notification.
cc_library(
    name = "stub_supervisor_client",
    srcs = ["supervisor_client/stub_supervisor_client.cpp"],
    deps = [":health_monitor_base"],
    alwayslink = True,
)

# Supervisor client backend that :health_monitor links.
# Built in: :alive_supervisor_client (default) and :stub_supervisor_client.
# An external backend is a cc_library that depends on :supervisor_client_interface and defines SupervisorClient::Create().
# Its visibility must include this package.
# Every backend sets alwayslink: only :health_monitor_base calls Create(), and a static link may place it after the backend.
label_flag(
    name = "supervisor_client",
    build_setting_default = ":alive_supervisor_client",
)

cc_library(
    name = "health_monitor",
    visibility = ["//visibility:public"],
    deps = [
        ":health_monitor_base",
        ":supervisor_client",
    ],
)

# gmock classes for tests of application code that takes the health monitor interfaces.
cc_library(
    name = "mock",
    testonly = True,
    hdrs = [
        "mock_deadline.h",
        "mock_deadline_monitor.h",
        "mock_health_monitor.h",
        "mock_heartbeat_monitor.h",
        "mock_logic_monitor.h",
        "mock_taken.h",
    ],
    include_prefix = "score/health_monitor",
    strip_include_prefix = "",
    visibility = ["//visibility:public"],
    deps = [
        ":health_monitor_base",
        "@googletest//:gtest",
    ],
)

# Links the stub backend directly, so the tests pass under every value of :supervisor_client.
cc_gtest_unit_test(
    name = "tests",
    srcs = [
        "deadline_test.cpp",
        "error_test.cpp",
        "health_monitor_builder_test.cpp",
        "heartbeat_monitor/heartbeat_monitor_impl_test.cpp",
        "heartbeat_monitor/heartbeat_state_test.cpp",
        "internal/fake_monotonic_time.h",
        "internal/fetch_update_test.cpp",
        "internal/health_monitor_impl_test.cpp",
        "internal/mock_supervisor_client.h",
        "internal/monitoring_logic_test.cpp",
        "internal/monotonic_time_test.cpp",
        "internal/test_helpers.h",
        "internal/thread_setup_test.cpp",
        "internal/worker_test.cpp",
        "mock_usage_test.cpp",
        "supervisor_client/stub_supervisor_client_test.cpp",
        "tag_test.cpp",
        "taken_test.cpp",
        "thread_parameters_test.cpp",
        "time_range_test.cpp",
    ],
    target_compatible_with = select({
        "//config:unit_qemu": [],
        "//config:unit_host": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [
        ":health_monitor_base",
        ":mock",
        ":stub_supervisor_client",
        "@score_baselibs//score/os/mocklib:time_mock",
    ],
)

# Separate from :tests, because a binary links one backend.
cc_gtest_unit_test(
    name = "alive_supervisor_client_tests",
    srcs = ["supervisor_client/alive_supervisor_client_test.cpp"],
    target_compatible_with = select({
        "//config:unit_qemu": [],
        "//config:unit_host": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [":alive_supervisor_client"],
)
