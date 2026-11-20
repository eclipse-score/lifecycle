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
import re

from tests.utils.testing_utils.logs import parse_dlt_file
from tests.utils.testing_utils.setup_test import setup_test
from tests.utils.testing_utils.run_test import run_test
from tests.utils.testing_utils.test_results import (
    assert_test_results,
    get_testcase_property,
)
from attribute_plugin import add_test_properties


@add_test_properties(
    partially_verifies=["comp_req__launch_man__process_log"],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_component_activation_log(
    target, setup_test, assert_test_results, remote_test_dir, test_output_dir
):
    """
    Objective: Verifies that the launch manager logs the launch and the
    termination of a component.

    A process is launched via the initial Run Target and records its own pid.
    The Launch Manager then shuts down, terminating the process.
    Expected Behaviour: The launch manager's DLT log contains an entry for the
    activation of the the component, and another entry for its termination.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/config.bin")],
        cwd=str(remote_test_dir),
    )

    # Also downloads the launch manager's LM.dlt to test_output_dir.
    assert_test_results({"logging_process.xml"})

    pid = get_testcase_property(test_output_dir / "logging_process.xml", "pid")
    logs = parse_dlt_file(test_output_dir / "LM.dlt")

    start_pattern = re.compile(r"Name:\s*component_logging\s*,\s*PID:\s*-?\d+\s*startProcess done")
    termination_pattern = re.compile(
        r"Name:\s*component_logging\s*,\s*PID:\s*(\d+)\s*Process terminated with status\s*(-?\d+)"
    )

    starts = [log for log in logs if start_pattern.search(log.message)]
    assert len(starts) == 1, (
        f"Expected one start log for component_logging, got {len(starts)} "
        f"in {len(logs)} log messages"
    )

    terminations = [
        match for log in logs if (match := termination_pattern.search(log.message))
    ]
    assert len(terminations) == 1, (
        f"Expected one termination log, got {len(terminations)} "
        f"in {len(logs)} log messages"
    )
    assert terminations[0].group(1) == pid, (
            f"Termination log reports the wrong pid (got: {terminations[0].group(1)}, expected: {pid})"
    )
    assert terminations[0].group(2) == "0", (
            f"Termination log reports the wrong status (got: {terminations[0].group(2)}, expected 0)"
    )
