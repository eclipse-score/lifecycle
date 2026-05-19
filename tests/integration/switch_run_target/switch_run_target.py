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
    check_for_failures,
    download_xml_results,
)
from attribute_plugin import add_test_properties


@add_test_properties(
    fully_verifies=[
        "feat_req__lifecycle__request_run_target_start",
        "feat_req__lifecycle__switch_run_targets",
        "feat_req__lifecycle__process_state_comm",
        "feat_req__lifecycle__process_termination",
        "feat_req__lifecycle__terminationn_dependency",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_switch_run_target(target, setup_test, test_output_dir, remote_test_dir):
    """
    Objective: Verifies that the launch manager respects component and run target dependencies when switching run targets, enforcing correct startup and termination order.

    The control client activates run_target_a, which depends on run_target_c (containing component_d) and component_a (which depends on component_b). After activation it switches back to Startup and then Off.
    Expected Behaviour: Component B starts before component A, component D is started, component A terminates before component B, and component E (not in the dependency chain) is never launched.
    """
    run_until_file_deployed(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        file_path=remote_test_dir.parent / "test_end",
        cwd=str(remote_test_dir),
        timeout_s=2.0,
    )

    download_xml_results(target, remote_test_dir, test_output_dir)
    all_files, failing_files = check_for_failures(test_output_dir)
    # Process C never starts so there should only be 4 files
    assert len(all_files) == 4, f"Didn't find the expected number of files {all_files}"
    assert len(failing_files) == 0, f"Found failures in files {failing_files}"
