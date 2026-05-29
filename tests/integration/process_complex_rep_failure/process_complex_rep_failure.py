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
from tests.utils.testing_utils.setup_test import setup_test
from tests.utils.testing_utils.test_results import (
    assert_test_results
)
from attribute_plugin import add_test_properties


@add_test_properties(
    partially_verifies=[
        "feat_req__lifecycle__controlif_status",
        "feat_req__lifecycle__process_failure_react",
        "feat_req__lifecycle__configurable_wait_time",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_recovery_action_complex_rep_failure(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies that recovery action is executed when the reporting of kRunning via LifecycleClient library (named "complex reporting" in the following) is not happening in time and vice versa.

    Case 1: Using complex reporting, the process does report kRunning in time (500ms below boundary)
    Expected Behaviour: Reporting kRunning is successful, recovery action is not executed.

    Case 2: Using complex reporting, the process does not report kRunning in time (500ms above boundary)
    Expected Behaviour: Reporting kRunning is not successful, recovery action is executed.
    """

    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        timeout_s=10.0,
    )

    assert_test_results({"control_client_mock.xml", "complex_reporting_process.xml"})
