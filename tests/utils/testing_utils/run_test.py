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


def run_test(*, target, binary_path, args: list[str] = [], cwd="/", timeout=15):
    """Run an integration test to completion."""

    test_runner = environ.get("SCORE_TEST_RUNNER", None)
    if test_runner:
        args.insert(0, binary_path)
        binary_path = test_runner

    process = target.execute_async(binary_path, args=args, cwd=cwd)
    assert process.wait(timeout) == 0
