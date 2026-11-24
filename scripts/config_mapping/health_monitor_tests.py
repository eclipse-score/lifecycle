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

import json

import jsonschema

from scripts.config_mapping import generate_health_monitor_schema


def test_schema_matches_fbs(generated_schema, schema_file):
    """The checked-in schema is the one generated from the .fbs."""
    generated = generate_health_monitor_schema.generate(generated_schema)
    with open(schema_file, encoding="utf-8") as handle:
        committed = handle.read()
    assert committed == generated, (
        "health_monitor_config_schema.json is out of sync with health_monitor_config.fbs. "
        "Re-run: bazel run //scripts/config_mapping:generate_health_monitor_schema and commit the result."
    )


def test_example_is_valid(schema_file, example):
    """The example configuration is valid against the checked-in schema."""
    with open(schema_file, encoding="utf-8") as handle:
        schema = json.load(handle)
    with open(example, encoding="utf-8") as handle:
        config = json.load(handle)
    jsonschema.Draft202012Validator.check_schema(schema)
    jsonschema.Draft202012Validator(schema).validate(config)
