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
from os import environ
from typing import Optional


def run_test(
    *, target, binary_path, args: Optional[list[str]] = None, cwd="/", timeout=15
):
    """Run an integration test to completion."""

    local_args = []
    if args:
        local_args = args

    test_runner = environ.get("SCORE_TEST_RUNNER", None)
    if test_runner:
        local_args.insert(0, binary_path)
        binary_path = test_runner

    process = target.execute_async(binary_path, args=local_args, cwd=cwd)
    assert process.wait(timeout) == 0


def run_test_non_root(
    *,
    target,
    binary_path,
    uid: int,
    gid: int,
    capabilities: Optional[list[str]] = None,
    args: Optional[list[str]] = None,
    cwd="/",
    timeout=15,
):
    """Run an integration test to completion as any given user.

    @param uid user Id to run as.
    @param gid group Id to run as.
    @param capabilities Capability names.
    """
    caps = ",".join(f"+{cap}" for cap in capabilities or [])
    setpriv_args = [f"--reuid={uid}", f"--regid={gid}", "--clear-groups"]
    if caps:
        setpriv_args += [f"--inh-caps={caps}", f"--ambient-caps={caps}"]

    run_test(
        target=target,
        binary_path="setpriv",
        args=setpriv_args + [str(binary_path)] + list(args or []),
        cwd=cwd,
        timeout=timeout,
    )
