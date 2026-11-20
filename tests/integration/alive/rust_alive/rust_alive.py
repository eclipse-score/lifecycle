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
    partially_verifies=[
        "comp_req__launch_man__report_lang_support",
        "comp_req__launch_man__alive_failure",
        "comp_req__launch_man__report_liveliness",
    ],
    test_type="requirements-based",
    derivation_technique="error-guessing",
)
def test_rust_alive(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies that an alive supervision failure is detected for a
    Rust application, and that correct reporting is not flagged.

    1: A Rust component reports alive 5 times and then stops reporting, while
       staying alive (too few notifications).
       Expected Behaviour: Alive supervision fails, the fallback
       Run Target is activated.
    2: A Rust component reports alive much more often than max_indications allows
       (too many notifications).
       Expected Behaviour: Alive supervision fails, the fallback Run Target is
       activated again.
    3: A Rust component keeps reporting alive within min/max_indications.
       Expected Behaviour: The fallback Run Target is not activated.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/rust_alive.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results({"control_client_test_driver.xml"})
