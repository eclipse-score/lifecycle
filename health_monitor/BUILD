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
load("//config:common_cc.bzl", "cc_binary_with_common_opts", "cc_library_with_common_opts")

cc_library_with_common_opts(
    name = "phm_logging",
    srcs = ["source/score/lcm/saf/logging/PhmLogger.cpp"],
    hdrs = ["source/score/lcm/saf/logging/PhmLogger.hpp"],
    includes = [
        "source",
        "source/score/lcm/saf/logging",
    ],
    deps = [
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "ifexm",
    srcs = [
        "source/score/lcm/saf/ifexm/ProcessCfg.cpp",
        "source/score/lcm/saf/ifexm/ProcessState.cpp",
        "source/score/lcm/saf/ifexm/ProcessStateReader.cpp",
    ],
    hdrs = glob(["source/score/lcm/saf/**/*.hpp"]),
    includes = [
        "include",
        "source",
    ],
    deps = [
        "//launch_manager:process_state_client",
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "ipc_if",
    hdrs = glob(["source/score/lcm/saf/ipc/*.hpp"]),
    deps = ["//external/ipc_dropin"],
    alwayslink = True,
)

cc_library_with_common_opts(
    name = "timers",
    srcs = [
        "source/score/lcm/saf/timers/CycleTimeValidator.cpp",
        "source/score/lcm/saf/timers/CycleTimer.cpp",
        "source/score/lcm/saf/timers/TimeConversion.cpp",
        "source/score/lcm/saf/timers/Timers_OsClock.cpp",
    ],
    hdrs = glob(["source/score/lcm/saf/timers/*.hpp"]),
    includes = [
        "source",
        "source/score/lcm/saf/timers",
    ],
)

cc_library_with_common_opts(
    name = "rb-watchdog-if",
    srcs = [
        "source/score/lcm/saf/watchdog/DeviceIf.cpp",
        "source/score/lcm/saf/watchdog/WatchdogImpl.cpp",
    ],
    hdrs = [
        "source/score/lcm/saf/logging/PhmLogger.hpp",
        "source/score/lcm/saf/timers/OsClockInterface.hpp",
        "source/score/lcm/saf/watchdog/DeviceIf.hpp",
        "source/score/lcm/saf/watchdog/IDeviceConfigFactory.hpp",
        "source/score/lcm/saf/watchdog/IWatchdogIf.hpp",
        "source/score/lcm/saf/watchdog/Watchdog.hpp",
        "source/score/lcm/saf/watchdog/WatchdogImpl.hpp",
    ],
    includes = [
        "include",
        "source",
        "source/score/lcm/saf/logging",
    ],
    deps = [
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "recoverynotify",
    srcs = ["source/score/lcm/saf/recovery/Notification.cpp"],
    hdrs = [
        "source/score/lcm/saf/logging/PhmLogger.hpp",
        "source/score/lcm/saf/recovery/Notification.hpp",
        "source/score/lcm/saf/timers/TimeConversion.hpp",
        "source/score/lcm/saf/timers/Timers_OsClock.hpp",
    ],
    includes = [
        "include",
        "source",
        "source/config/gen/hm_model",
        "source/config/gen/hmcore_model",
    ],
    deps = [
        "//launch_manager:recovery_client",
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "factory",
    srcs = [
        "source/score/lcm/saf/factory/FlatCfgFactory.cpp",
        "source/score/lcm/saf/factory/MachineConfigFactory.cpp",
    ],
    hdrs = glob([
        "source/score/lcm/saf/**/*.hpp",
        "source/config/gen/**/*.h",
        "include/**/*.h",
    ]),
    includes = [
        "include",
        "source",
    ],
    deps = [
        ":recoverynotify",
        "//external/ipc_dropin",
        "//launch_manager:process_state_client",
        "@flatbuffers",
    ] + select(
        {
            "//config:x86_64-qnx": [],
            "//conditions:default": [
                "//external/acl",
            ],
        },
    ),
)

cc_library_with_common_opts(
    name = "ifappl",
    srcs = [
        "source/score/lcm/saf/ifappl/Checkpoint.cpp",
        "source/score/lcm/saf/ifappl/MonitorIfDaemon.cpp",
    ],
    hdrs = glob(["source/score/lcm/saf/**/*.hpp"]),
    includes = [
        "include",
        "source",
        "source/score/lcm/saf/logging",
    ],
    deps = [
        "//external/ipc_dropin",
        "//launch_manager:process_state_client",
    ] + select(
        {
            "//config:x86_64-qnx": [],
            "//conditions:default": [
                "//external/acl",
            ],
        },
    ),
)

cc_library_with_common_opts(
    name = "supervision",
    srcs = [
        "source/score/lcm/saf/supervision/Alive.cpp",
        "source/score/lcm/saf/supervision/Deadline.cpp",
        "source/score/lcm/saf/supervision/Global.cpp",
        "source/score/lcm/saf/supervision/ICheckpointSupervision.cpp",
        "source/score/lcm/saf/supervision/ISupervision.cpp",
        "source/score/lcm/saf/supervision/Local.cpp",
        "source/score/lcm/saf/supervision/Logical.cpp",
        "source/score/lcm/saf/supervision/ProcessStateTracker.cpp",
    ],
    hdrs = glob([
        "source/score/lcm/saf/**/*.hpp",
        "include/score/lcm/*.h",
    ]),
    includes = [
        "include",
        "source/score/lcm/saf/supervision",
    ],
    deps = [":recoverynotify"],
)

cc_library_with_common_opts(
    name = "hm-lib",
    srcs = [
        "source/score/lcm/hmlib/Monitor.cpp",
        "source/score/lcm/hmlib/MonitorImpl.cpp",
        "source/score/lcm/hmlib/MonitorImplWrapper.cpp",
    ],
    hdrs = glob([
        "source/score/lcm/**/*.h",
        "include/score/**/*.h",
        "source/score/lcm/saf/**/*.hpp",
        "source/config/gen/**/*.h",
    ]),
    includes = [
        "include",
        "source",
        "source/config/gen/hm_model",
        "source/config/gen/hmcore_model",
    ],
    deps = [
        ":ipc_if",
        ":phm_logging",
        "@flatbuffers",
        "@flatbuffers//:flatc",
    ] + select(
        {
            "//config:x86_64-qnx": [],
            "//conditions:default": [
                "//external/acl",
            ],
        },
    ),
)

cc_library_with_common_opts(
    name = "daemon",
    srcs = [
        "source/score/lcm/saf/daemon/PhmDaemon.cpp",
        "source/score/lcm/saf/daemon/SwClusterHandler.cpp",
    ],
    cxxopts = [
        "-fpic",
        "-Werror",
    ],
    includes = [
        "include",
        "source",
    ],
    deps = [
        ":factory",
        ":rb-watchdog-if",
        "//external/ipc_dropin",
        "//launch_manager:lifecycle_client",
    ],
)

cc_library_with_common_opts(
    name = "health_monitor",
    srcs = [
        "source/score/lcm/saf/daemon/health_monitor.cpp",
    ],
    cxxopts = [
        "-fpic",
        "-Werror",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":daemon",
        ":factory",
        ":ifappl",
        ":ifexm",
        ":ipc_if",
        ":phm_logging",
        ":rb-watchdog-if",
        ":recoverynotify",
        ":supervision",
        ":timers",
        "@flatbuffers//:flatc",
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "hm_shared_lib",
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":hm-lib",
        ":ipc_if",
        ":phm_logging",
        ":timers",
        "@flatbuffers",
    ],
    alwayslink = True,
)
