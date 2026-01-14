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

# flatcfg configuration
cc_library(
    name = "config",
    hdrs = [
        "config/hm_flatcfg_generated.h",
        "config/hmcore_flatcfg_generated.h",
    ],
    includes = ["config"],
    visibility = ["//src:__pkg__"],
)

# library for logging
cc_library_with_common_opts(
    name = "phm_logging",
    srcs = ["src/score/lcm/saf/logging/PhmLogger.cpp"],
    hdrs = ["src/score/lcm/saf/logging/PhmLogger.hpp"],
    includes = [
        "src",
        "src/score/lcm/saf/logging",
    ],
    visibility = ["//src:__pkg__"],
    deps = [
        "@score_baselibs//score/mw/log",
    ],
)

# wrapper for interacting with lcm
cc_library_with_common_opts(
    name = "ifexm",
    srcs = [
        "src/score/lcm/saf/ifexm/ProcessCfg.cpp",
        "src/score/lcm/saf/ifexm/ProcessState.cpp",
        "src/score/lcm/saf/ifexm/ProcessStateReader.cpp",
    ],
    hdrs = glob(["src/score/lcm/saf/**/*.hpp"]),
    includes = [
        "include",
        "src",
    ],
    visibility = ["//src:__pkg__"],
    deps = [
        "//src/launch_manager_daemon/process_state_client_lib:process_state_client",
        "@score_baselibs//score/mw/log",
    ],
)

# wrapper for ipc
cc_library_with_common_opts(
    name = "ipc_if",
    hdrs = glob(["src/score/lcm/saf/ipc/*.hpp"]),
    visibility = ["//src:__pkg__"],
    deps = ["//externals/ipc_dropin"],
    alwayslink = True,
)

cc_library_with_common_opts(
    name = "timers",
    srcs = [
        "src/score/lcm/saf/timers/CycleTimeValidator.cpp",
        "src/score/lcm/saf/timers/CycleTimer.cpp",
        "src/score/lcm/saf/timers/TimeConversion.cpp",
        "src/score/lcm/saf/timers/Timers_OsClock.cpp",
    ],
    hdrs = glob(["src/score/lcm/saf/timers/*.hpp"]),
    includes = [
        "src",
        "src/score/lcm/saf/timers",
    ],
    visibility = ["//src:__pkg__"],
)

cc_library_with_common_opts(
    name = "rb-watchdog-if",
    srcs = [
        "src/score/lcm/saf/watchdog/DeviceIf.cpp",
        "src/score/lcm/saf/watchdog/WatchdogImpl.cpp",
    ],
    hdrs = [
        "src/score/lcm/saf/logging/PhmLogger.hpp",
        "src/score/lcm/saf/timers/OsClockInterface.hpp",
        "src/score/lcm/saf/watchdog/DeviceIf.hpp",
        "src/score/lcm/saf/watchdog/IDeviceConfigFactory.hpp",
        "src/score/lcm/saf/watchdog/IWatchdogIf.hpp",
        "src/score/lcm/saf/watchdog/Watchdog.hpp",
        "src/score/lcm/saf/watchdog/WatchdogImpl.hpp",
    ],
    includes = [
        "include",
        "src",
        "src/score/lcm/saf/logging",
    ],
    deps = [
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "recoverynotify",
    srcs = ["src/score/lcm/saf/recovery/Notification.cpp"],
    hdrs = [
        "src/score/lcm/saf/logging/PhmLogger.hpp",
        "src/score/lcm/saf/recovery/Notification.hpp",
        "src/score/lcm/saf/timers/TimeConversion.hpp",
        "src/score/lcm/saf/timers/Timers_OsClock.hpp",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        ":config",
        "//src/launch_manager_daemon/recovery_client_lib:recovery_client",
        "@score_baselibs//score/mw/log",
    ],
)

cc_library_with_common_opts(
    name = "factory",
    srcs = [
        "src/score/lcm/saf/factory/FlatCfgFactory.cpp",
        "src/score/lcm/saf/factory/MachineConfigFactory.cpp",
    ],
    hdrs = glob([
        "src/score/lcm/saf/**/*.hpp",
        "include/**/*.h",
    ]),
    includes = [
        "include",
        "src",
    ],
    visibility = ["//src:__pkg__"],
    deps = [
        ":config",
        ":recoverynotify",
        "//externals/ipc_dropin",
        "//src/launch_manager_daemon/process_state_client_lib:process_state_client",
        "@flatbuffers",
    ] + select(
        {
            "//config:build_qnx8": [],
            "//config:x86_64-qnx": [],
            "//conditions:default": [
                "//externals/acl",
            ],
        },
    ),
)

cc_library_with_common_opts(
    name = "ifappl",
    srcs = [
        "src/score/lcm/saf/ifappl/Checkpoint.cpp",
        "src/score/lcm/saf/ifappl/MonitorIfDaemon.cpp",
    ],
    hdrs = glob(["src/score/lcm/saf/**/*.hpp"]),
    includes = [
        "include",
        "src",
        "src/score/lcm/saf/logging",
    ],
    visibility = ["//src:__pkg__"],
    deps = [
        "//externals/ipc_dropin",
        "//src/launch_manager_daemon/process_state_client_lib:process_state_client",
    ] + select(
        {
            "//config:build_qnx8": [],
            "//config:x86_64-qnx": [],
            "//conditions:default": [
                "//externals/acl",
            ],
        },
    ),
)

cc_library_with_common_opts(
    name = "supervision",
    srcs = [
        "src/score/lcm/saf/supervision/Alive.cpp",
        "src/score/lcm/saf/supervision/Deadline.cpp",
        "src/score/lcm/saf/supervision/Global.cpp",
        "src/score/lcm/saf/supervision/ICheckpointSupervision.cpp",
        "src/score/lcm/saf/supervision/ISupervision.cpp",
        "src/score/lcm/saf/supervision/Local.cpp",
        "src/score/lcm/saf/supervision/Logical.cpp",
        "src/score/lcm/saf/supervision/ProcessStateTracker.cpp",
    ],
    hdrs = glob([
        "src/score/lcm/saf/**/*.hpp",
        "include/score/lcm/*.h",
    ]),
    includes = [
        "include",
        "src/score/lcm/saf/supervision",
    ],
    visibility = ["//src:__pkg__"],
    deps = [
        ":recoverynotify",
        "//src/launch_manager_daemon/process_state_client_lib:process_state_client",
    ],
)

cc_library_with_common_opts(
    name = "hm-lib",
    srcs = [
        "src/score/lcm/hmlib/Monitor.cpp",
        "src/score/lcm/hmlib/MonitorImpl.cpp",
        "src/score/lcm/hmlib/MonitorImplWrapper.cpp",
    ],
    hdrs = glob([
        "src/score/lcm/**/*.h",
        "include/score/**/*.h",
        "src/score/lcm/saf/**/*.hpp",
    ]),
    includes = [
        "include",
        "src",
    ],
    visibility = ["//src:__subpackages__"],
    deps = [
        ":config",
        ":ipc_if",
        ":phm_logging",
        "@flatbuffers",
        "@flatbuffers//:flatc",
    ] + select(
        {
            "//config:build_qnx8": [],
            "//config:x86_64-qnx": [],
            "//conditions:default": [
                "//externals/acl",
            ],
        },
    ),
)

cc_library_with_common_opts(
    name = "daemon",
    srcs = [
        "src/score/lcm/saf/daemon/PhmDaemon.cpp",
        "src/score/lcm/saf/daemon/SwClusterHandler.cpp",
    ],
    cxxopts = [
        "-fpic",
        "-Werror",
    ],
    includes = [
        "include",
        "src",
    ],
    visibility = ["//src:__pkg__"],
    deps = [
        ":factory",
        ":rb-watchdog-if",
        "//externals/ipc_dropin",
        "//src/control_client_lib",
        "//src/launch_manager_daemon/lifecycle_client_lib:lifecycle_client",
    ],
)

cc_library_with_common_opts(
    name = "health_monitor",
    srcs = [
        "src/score/lcm/saf/daemon/HealthMonitorImpl.cpp",
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
