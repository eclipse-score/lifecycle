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
def test_alive_failure(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies that a missing alive notification is detected for both
    a C++ and a Rust application.

    1: A C++ component reports alive 5 times and then stops reporting, while
       staying alive.
       Expected Behaviour: Alive supervision fails, the fallback
       Run Target is activated.
    2: The same with a Rust component.
       Expected Behaviour: Alive supervision fails, the fallback Run Target is
       activated again.
    3: A C++ and a Rust component keep reporting alive.
       Expected Behaviour: The fallback Run Target is not activated.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/alive_failure.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results({"control_client_test_driver.xml"})
