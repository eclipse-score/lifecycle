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
"""Checks of the health monitor configuration schema.

* The checked-in ``health_monitor_config_schema.json`` is byte-identical to the schema generated
  from ``health_monitor_config.fbs``.
* The example configuration is valid against the checked-in schema.
"""

import argparse
import json
import os
import sys
import unittest

import jsonschema

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import generate_schema  # noqa: E402

_ARGS = None


class SchemaTest(unittest.TestCase):
    def test_schema_matches_fbs(self):
        """The checked-in schema is the one generated from the .fbs."""
        generated = generate_schema.generate(_ARGS.generated)
        with open(_ARGS.schema, encoding="utf-8") as handle:
            committed = handle.read()
        self.assertEqual(
            committed,
            generated,
            "health_monitor_config_schema.json is out of sync with health_monitor_config.fbs. "
            "Re-run: bazel run //score/health_monitor/configuration:generate_schema and commit the result.",
        )

    def test_example_is_valid(self):
        """The example configuration is valid against the checked-in schema."""
        with open(_ARGS.schema, encoding="utf-8") as handle:
            schema = json.load(handle)
        with open(_ARGS.example, encoding="utf-8") as handle:
            example = json.load(handle)
        jsonschema.Draft202012Validator.check_schema(schema)
        jsonschema.Draft202012Validator(schema).validate(example)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--generated", required=True, help="Path to the generate_json_schema output."
    )
    parser.add_argument(
        "--schema", required=True, help="Path to the checked-in schema."
    )
    parser.add_argument(
        "--example", required=True, help="Path to the example configuration."
    )
    _ARGS, remaining = parser.parse_known_args()
    unittest.main(argv=[sys.argv[0]] + remaining)
