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
from tests.integration.testing_utils import (
    get_common_interface,
    check_for_failures,
    format_logs,
)
from pathlib import Path
from attribute_plugin import add_test_properties

@add_test_properties(
        partially_verifies=[],
        test_type="interface-test",
        derivation_technique="explorative-testing",
)
def test_smoke():
    """Smoke test for the launch manager daemon."""
    code, stdout, stderr = get_common_interface().run_until_file_deployed(
        "src/launch_manager_daemon/launch_manager"
    )

    print(format_logs(code, stdout, stderr))

    check_for_failures(Path("tests/integration/smoke"), 2)
    assert code == 0
