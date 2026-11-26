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
from tests.utils.testing_utils.setup_test import setup_test
from tests.utils.testing_utils.run_test import run_test
from tests.utils.testing_utils.test_results import assert_test_results
from attribute_plugin import add_test_properties


@add_test_properties(
    fully_verifies=["comp_req__launch_man__multi_start_support"],
    partially_verifies=["feat_req__lifecycle__running_processes"],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_multi_start(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies that the launch manager can launch the same executable more than once.

    Three components use the same executable and are all part of one Run Target, which is activated by the
    control client.
    Expected Behaviour: The launch manager starts three processes of the executable, which all run at the same
    time and report running, so the Run Target is activated. Each process writes its own result file.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/config.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results(
        {
            "control_client_test_driver.xml",
            "reporting_process_instance_1.xml",
            "reporting_process_instance_2.xml",
            "reporting_process_instance_3.xml",
        }
    )
