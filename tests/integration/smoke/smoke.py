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
import logging
from tests.utils.testing_utils.run_until_file_deployed import run_until_file_deployed
from tests.utils.testing_utils.setup_test import setup_test
from tests.utils.testing_utils.test_results import (
    check_for_failures,
    download_xml_results,
)
from attribute_plugin import add_test_properties


@add_test_properties(
    partially_verifies=[],
    test_type="interface-test",
    derivation_technique="explorative-testing",
)
def test_smoke(target, setup_test, test_output_dir, remote_test_dir):
    """Smoke test for the launch manager daemon running inside a Docker container."""
    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        timeout_s=2.0,
    )

    download_xml_results(target, remote_test_dir, test_output_dir)
    all_files, failing_files = check_for_failures(test_output_dir)
    assert len(all_files) == 2, f"Didn't find the expected number of files {all_files}"
    assert len(failing_files) == 0, f"Found failures in files {failing_files}"
