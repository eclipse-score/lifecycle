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
from dataclasses import dataclass
from os import environ
from typing import Optional

# File touched if a test has failed.
TEST_FAILED_FILE = "test_failed"


@dataclass(frozen=True)
class RunAs:
    """Identity to run a test binary as, via `setpriv`.

    @param uid user Id to run as.
    @param gid group Id to run as.
    @param capabilities Capability names, set as inheritable and ambient.
    """

    uid: int
    gid: int
    capabilities: Optional[list[str]] = None


def _setpriv_command(*, run_as: RunAs, binary_path, args):
    caps = ",".join(f"+{cap}" for cap in run_as.capabilities or [])
    setpriv_args = [f"--reuid={run_as.uid}", f"--regid={run_as.gid}", "--clear-groups"]
    if caps:
        setpriv_args += [f"--inh-caps={caps}", f"--ambient-caps={caps}"]
    return setpriv_args + [str(binary_path)] + list(args or [])


def run_test(
    *,
    target,
    binary_path,
    args: Optional[list[str]] = None,
    cwd="/",
    timeout=15,
    run_as: Optional[RunAs] = None,
    expect_failure=False,
):
    """Run an integration test to completion.

    @param run_as If set, run the binary as this identity using `setpriv`.
    @param expect_failure If True, expect the fallback run target to have run
                          `fail_test` (the `test_failed` file exists), otherwise
                          expect exit code 0 and no `test_failed` file.

    @details `fail_test` ends the Launch Manager with SIGTERM, which shuts it down
             gracefully, so an expected failure is detected by the `test_failed`
             file and not by the exit code.
    """
    local_args = list(args) if args else []

    if run_as is not None:
        local_args = _setpriv_command(
            run_as=run_as, binary_path=binary_path, args=local_args
        )
        binary_path = "setpriv"

    test_runner = environ.get("SCORE_TEST_RUNNER", None)
    if test_runner:
        local_args.insert(0, binary_path)
        binary_path = test_runner

    process = target.execute_async(binary_path, args=local_args, cwd=cwd)
    exit_code = process.wait(timeout)

    failed_explicitly = target.execute(f"test -e {cwd}/{TEST_FAILED_FILE}")[0] == 0

    if expect_failure:
        assert failed_explicitly, (
            f"Expected fail_test to create '{TEST_FAILED_FILE}', but it does not exist"
        )
        return

    assert exit_code == 0, f"Expected exit code 0, got {exit_code}"
    assert not failed_explicitly, (
        f"A component failed the test explicitly (fail_test created '{TEST_FAILED_FILE}')"
    )
