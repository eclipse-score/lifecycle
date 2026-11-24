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
"""Write ``health_monitor_config_schema.json`` from the FlatBuffers schema.

Baselibs' ``generate_json_schema`` derives a JSON schema from ``health_monitor_config.fbs`` with ``flatc``.
``--in`` takes its output, and this tool copies that output over the checked-in schema.
``health_monitor_tests`` compares the two byte for byte.

Run it with:
    bazel run //scripts/config_mapping:generate_health_monitor_schema
"""

import argparse
import os
import sys


def generate(postproc_schema_path):
    """Return the schema text generated from the ``.fbs``."""
    with open(postproc_schema_path, "r", encoding="utf-8") as handle:
        return handle.read()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--in",
        dest="input",
        required=True,
        help="Path to the generate_json_schema output.",
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Where to write the schema. A relative path is resolved against BUILD_WORKSPACE_DIRECTORY, "
        "so that `bazel run` updates the source tree.",
    )
    args = parser.parse_args(sys.argv[1:] if argv is None else argv)

    output = args.output
    if not os.path.isabs(output):
        workspace = os.environ.get("BUILD_WORKSPACE_DIRECTORY")
        if not workspace:
            raise SystemExit(
                "BUILD_WORKSPACE_DIRECTORY is not set. Run with `bazel run`, or pass an absolute --output."
            )
        output = os.path.join(workspace, output)

    with open(output, "w", encoding="utf-8") as handle:
        handle.write(generate(args.input))
    return 0


if __name__ == "__main__":
    sys.exit(main())
