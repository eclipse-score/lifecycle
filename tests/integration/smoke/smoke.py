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
from tests.utils.testing_utils.run_until_file_deployed import run_until_file_deployed
from tests.utils.testing_utils.setup_test import setup_tests
from tests.utils.testing_utils.test_results import check_for_failures


def test_smoke(target, setup_tests, test_output_dir, remote_test_dir):
    """Smoke test for the launch manager daemon running inside a Docker container."""
    test_end_file = str(remote_test_dir.parent / "test_end")

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=test_end_file,
        cwd=str(remote_test_dir),
        timeout_s=60.0,
    )

    local_results = test_output_dir / "results"
    local_results.mkdir(exist_ok=True)
    for xml_name in ("control_daemon_mock.xml", "gtest_process.xml"):
        target.download(str(remote_test_dir / xml_name), str(local_results / xml_name))

    check_for_failures(local_results, 2)
