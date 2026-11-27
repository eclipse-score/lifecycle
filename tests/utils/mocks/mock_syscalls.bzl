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

# Symbols for which mock_syscalls.hpp defines a __wrap_<symbol> that calls __real_<symbol>.
# Every one of them must be wrapped, otherwise __real_<symbol> is unresolved at link time.
# Keep in sync with mock_syscalls.hpp.
MOCK_SYSCALLS_WRAPPED_SYMBOLS = [
    "fork",
    "execve",
    "kill",
    "wait",
    "access",
    "shm_open",
    "ftruncate",
    "shm_unlink",
    "mmap",
    "munmap",
    "setpgid",
    "setgid",
    "setuid",
    "sched_setscheduler",
    "chdir",
    "setrlimit",
    "getpid",
    "fcntl",
    "sem_destroy",
    "msync",
    "close",
    "fstat",
    "clock_gettime",
    "clock_getres",
]

def linker_wrap_symbols(symbols):
    """Returns linkopts that make the linker wrap each of the given symbols.

    Args:
        symbols: List of symbol names to wrap.

    Returns:
        List of "-Wl,--wrap=<symbol>" linker options.
    """
    return ["-Wl,--wrap=" + symbol for symbol in symbols]
