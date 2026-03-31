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


def download_xml_results(target, remote_dir: Path, local_dir: Path):
    """Glob all .xml files in remote_dir, download them to local_dir, and delete
    them from the remote.
    """
    res, stdout = target.execute(f"ls {remote_dir}/*.xml")
    assert res == 0, "Couldn't get list of files"
    remote_xml_files = stdout.decode().strip().splitlines()
    for remote_path in remote_xml_files:
        remote_path = remote_path.strip()
        if not remote_path:
            continue
        xml_name = Path(remote_path).name
        try:
            target.download(remote_path, str(local_dir / xml_name))
            target.execute(f"rm {remote_path}")
        except Exception:
            pass


def check_for_failures(path: Path):
    """Check expected_count xml files for failures, raising an exception if
    a failure is found or a different number of xml files are found.
    """
    failing_files = []
    all_files = []
    for file in path.glob("*.xml"):
        xml = JUnitXml.fromfile(str(file))
        if xml.failures > 0 or xml.errors > 0:
            failing_files.append(file.name)
        all_files.append(file.name)

    return all_files, failing_files
