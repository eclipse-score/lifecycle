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
from attribute_plugin import add_test_properties


@add_test_properties(
    fully_verifies=[
        "comp_req__launch_man__report_lang_support",
        "comp_req__launch_man__report_liveliness",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_alive_success(target, setup_test, remote_test_dir):
    """
    Objective: Verifies that the launch manager supports C++ and Rust
    applications reporting their liveliness via the Alive interface.

    Expected Behaviour: No alive supervision failure is detected for either
    component, so the fallback Run Target is not activated.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/alive_success.bin")],
        cwd=str(remote_test_dir),
    )
