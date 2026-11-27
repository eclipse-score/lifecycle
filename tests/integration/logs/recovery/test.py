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
from tests.utils.testing_utils.test_results import assert_test_results
from attribute_plugin import add_test_properties


@add_test_properties(
    partially_verifies=["comp_req__launch_man__ra_log"],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_component_recovery_log(
    target, setup_test, assert_test_results, remote_test_dir, test_output_dir
):
    """
    Objective: Verifies that the launch manager logs a recovery action.

    The initial Run Target contains a component whose binary does not exist, so
    its activation fails and the launch manager switches to the fallback Run
    Target.
    The fallback Run Target runs a process which ends the test.
    Expected Behaviour: The launch manager's DLT log contains exactly one
    warning that the recovery state is activated, followed by the start of the
    transition to the fallback Run Target.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/config.bin")],
        cwd=str(remote_test_dir),
    )

    # no xml results should be made
    assert_test_results(set())

    logs = parse_dlt_file(test_output_dir / "LM.dlt")

    recovery_pattern = re.compile(
        r"Problem discovered, activating recovery state:\s*\S+"
    )
    recoveries = [log for log in logs if recovery_pattern.search(log.message)]
    assert len(recoveries) == 1, (
        f"Expected one recovery log, got {len(recoveries)} in {len(logs)} log messages"
    )
    assert recoveries[0].level == "warn", (
        f"Expected the recovery log to be a warning, got {recoveries[0].level}"
    )
