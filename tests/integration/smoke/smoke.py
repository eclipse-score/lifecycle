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
from tests.utils.fixtures import (
    file_interface,
    control_interface,
    target,
    setup_tests,
    download_test_results,
    test_dir,
    check_for_failures,
)
from pathlib import Path
from attribute_plugin import add_test_properties
import logging


@add_test_properties(
    partially_verifies=[],
    test_type="interface-test",
    derivation_technique="explorative-testing",
)
def test_smoke(setup_tests, control_interface, download_test_results, test_dir):
    """Smoke test for the launch manager daemon."""

    code, stdout, stderr = control_interface.run_until_file_deployed(
        "./launch_manager",
        Path("/opt/score/tests/smoke/test_end"),
        cwd="/opt/score/tests/smoke/bin",
        timeout=1,
    )
    logging.info(stdout)
    logging.info(stderr)

    assert code == 0, f"Exit code was not 0! {code} instead"

    download_test_results()
    check_for_failures(test_dir, 2)
