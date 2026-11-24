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
* ``health_monitor_config.py`` accepts the example and rejects configurations that ``flatc``
  would accept or that violate the schema otherwise.
"""

import json
from pathlib import Path

import jsonschema
import pytest

from scripts.config_mapping import generate_health_monitor_schema
from scripts.config_mapping import health_monitor_config

invalid_dir = Path(__file__).parent / "tests" / "health_monitor" / "invalid"


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


def test_valid_config_is_copied(schema_file, example, tmp_path):
    """A valid configuration is copied unchanged to <stem>_gen.json."""
    result = health_monitor_config.main(
        [example, "--schema", schema_file, "-o", str(tmp_path)]
    )
    assert result == health_monitor_config.SUCCESS
    output = tmp_path / "health_monitor_config_gen.json"
    assert output.read_bytes() == Path(example).read_bytes()


@pytest.mark.parametrize(
    "name, message",
    [
        ("missing_schema_version", "'schema_version' is a required property"),
        ("unsupported_schema_version", "2 is greater than the maximum of 1"),
        ("missing_min_ms", "'min_ms' is a required property"),
        ("missing_max_ms", "'max_ms' is a required property"),
        ("missing_monitor_tag", "'tag' is a required property"),
        (
            "unknown_key",
            "Additional properties are not allowed ('unknown_key' was unexpected)",
        ),
    ],
)
def test_invalid_config_is_rejected(schema_file, tmp_path, capsys, name, message):
    """An invalid configuration fails validation and produces no output."""
    result = health_monitor_config.main(
        [
            str(invalid_dir / f"{name}.json"),
            "--schema",
            schema_file,
            "-o",
            str(tmp_path),
        ]
    )
    assert result == health_monitor_config.SCHEMA_VALIDATION_FAILURE
    assert message in capsys.readouterr().err
    assert not any(tmp_path.iterdir())
