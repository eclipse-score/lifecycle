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
"""Validate a health monitor configuration against its JSON schema.

``flatc --binary`` rejects a missing ``(required)`` string, table or vector, but it accepts a
missing optional scalar (``field: T = null;``) and does not check value ranges. This tool runs
the JSON schema first. On success it writes ``<output_dir>/<stem>_gen.json``, an unchanged copy
of the input. ``serialize_buffer`` then reads that copy, so it only converts validated input.

Usage:
    health_monitor_config.py <configuration.json> --schema <schema.json> -o <output_dir>
    health_monitor_config.py <configuration.json> --schema <schema.json> --validate
"""

import argparse
import json
import os
import shutil
import sys

import jsonschema

# Possible exit codes returned from this script. The values match lifecycle_config.py.
SUCCESS = 0
SCHEMA_VALIDATION_FAILURE = 2


def schema_validation(config, schema, config_path, schema_path):
    """Return True if ``config`` is valid against ``schema``, and print every error otherwise."""
    validator = jsonschema.Draft202012Validator(schema)
    errors = sorted(
        validator.iter_errors(config),
        key=lambda err: [str(p) for p in err.absolute_path],
    )
    for err in errors:
        path = " -> ".join(str(p) for p in err.absolute_path)
        location = f" (at: {path})" if path else ""
        print(
            f"Error: Schema validation failed{location}: {err.message}", file=sys.stderr
        )
    if errors:
        print(
            f"Validated Config: {config_path}\nSchema File: {schema_path}",
            file=sys.stderr,
        )
        return False
    return True


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("filename", help="The health monitor configuration file")
    parser.add_argument(
        "--output-dir", "-o", default=".", help="Output directory for generated files"
    )
    parser.add_argument(
        "--validate",
        default=False,
        action="store_true",
        help="Only validate the provided configuration file against the schema and exit.",
    )
    parser.add_argument(
        "--schema", required=True, help="Path to the JSON schema file for validation"
    )
    args = parser.parse_args(argv)

    with open(args.filename, encoding="utf-8") as handle:
        config = json.load(handle)
    with open(args.schema, encoding="utf-8") as handle:
        schema = json.load(handle)

    if not schema_validation(config, schema, args.filename, args.schema):
        return SCHEMA_VALIDATION_FAILURE
    if args.validate:
        return SUCCESS

    stem = os.path.splitext(os.path.basename(args.filename))[0]
    os.makedirs(args.output_dir, exist_ok=True)
    shutil.copyfile(args.filename, os.path.join(args.output_dir, f"{stem}_gen.json"))
    return SUCCESS


if __name__ == "__main__":
    sys.exit(main())
