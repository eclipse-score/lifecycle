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
from pathlib import Path

from junitparser import JUnitXml


def check_for_failures(path: Path, expected_count: int):
    """Check expected_count xml files for failures, raising an exception if
    a failure is found or a different number of xml files are found.
    """
    failing_files = []
    checked_files = []
    for file in path.iterdir():
        if file.suffix == ".xml":
            xml = JUnitXml.fromfile(str(file))
            if xml.failures > 0 or xml.errors > 0:
                failing_files.append(file.name)
            checked_files.append(file.name)
    if len(failing_files) > 0:
        raise RuntimeError(
            f"Failures found in the following files:\n {'\n'.join(failing_files)}"
        )
    if len(checked_files) != expected_count:
        raise RuntimeError(
            f"Expected to find {expected_count} xml files, instead found {len(checked_files)}:\n{'\n'.join(checked_files)}"
            f"Checked in dir {path}"
        )
