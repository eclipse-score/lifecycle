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

import subprocess
import pytest
import shutil
import sys
import os
from pathlib import Path
import filecmp
from scripts.config_mapping.lifecycle_config import (
    SUCCESS,
    SCHEMA_VALIDATION_DEPENDENCY_ERROR,
    SCHEMA_VALIDATION_FAILURE,
    CUSTOM_VALIDATION_FAILURE,
)

script_dir = Path(__file__).parent
tests_dir = script_dir / "tests"
lifecycle_script = script_dir / "lifecycle_config.py"


def run(
    input_file: Path,
    test_name: str,
    schema_file: Path,
    compare_files_only=[],
    exclude_files=[],
    compare_output=True,
):
    """
    Execute the mapping script with the given input file and compare the generated output with the expected output.
    Input:
    - input_file: The path to the input JSON file for the mapping script
    - test_name: The name of the test case, which corresponds to a subdirectory in the "tests" directory containing the expected output
    """
    actual_output_dir = tests_dir / test_name / "actual_output"
    expected_output_dir = tests_dir / test_name / "expected_output"

    if compare_files_only and exclude_files:
        raise AssertionError(
            "You may only make use of either parameters: compare_files_only or exclude_files, but not both."
        )

    # Clean and create actual output directory
    if actual_output_dir.exists():
        shutil.rmtree(actual_output_dir)
    actual_output_dir.mkdir(parents=True)

    # Execute lifecycle_config.py
    cmd = [
        sys.executable,
        str(lifecycle_script),
        str(input_file),
        "-o",
        str(actual_output_dir),
        "--schema",
        str(schema_file),
    ]

    # Pass the parent process's sys.path so the subprocess can find packages
    # installed in the bazel virtualenv (e.g. jsonschema).
    env = os.environ.copy()
    env["PYTHONPATH"] = os.pathsep.join(sys.path)

    try:
        result = subprocess.run(
            cmd, check=True, capture_output=True, text=True, env=env
        )
        print(f"Command executed successfully: {' '.join(cmd)}")
        print(f"Output: {result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {' '.join(cmd)}")
        print(f"Error: {e.stderr}")
        raise

    if compare_output:
        if compare_files_only:
            # Compare only specific files
            if not compare_files(
                actual_output_dir, expected_output_dir, compare_files_only
            ):
                raise AssertionError(
                    "Actual output files do not match expected output files."
                )
        else:
            # Compare the complete directory content
            if not compare_directories(
                actual_output_dir, expected_output_dir, exclude_files
            ):
                raise AssertionError("Actual output does not match expected output.")
    return result


def compare_directories(dir1: Path, dir2: Path, exclude_files: list) -> bool:
    """
    Compare two directories recursively. Return True if they are the same, False otherwise.
    """
    dcmp = filecmp.dircmp(dir1, dir2, ignore=exclude_files)

    if dcmp.left_only or dcmp.right_only or dcmp.diff_files:
        print(f"Directories differ: {dir1} vs {dir2}")
        print(f"Only in {dir1}: {dcmp.left_only}")
        print(f"Only in {dir2}: {dcmp.right_only}")
        print(f"Different files: {dcmp.diff_files}")
        return False

    for common_dir in dcmp.common_dirs:
        if not compare_directories(dir1 / common_dir, dir2 / common_dir):
            return False

    return True


def compare_files(dir1: Path, dir2: Path, files: list) -> bool:
    """
    Compare specific files in two directories. Return True if they are the same, False otherwise.
    """
    for file in files:
        file1 = dir1 / file
        file2 = dir2 / file
        if not filecmp.cmp(file1, file2, shallow=False):
            print(f"Files differ: {file1} vs {file2}")
            return False
    return True


def test_smoke_test(schema_file):
    """
    Basic Smoketest for generating both launch manager and health monitoring configuration
    """

    test_name = "smoke_test"
    input_file = tests_dir / test_name / "input" / "lm_config.json"

    run(input_file, test_name, schema_file)


def test_minimal_config(schema_file):
    """
    Test generation of launch manager configuration
    with only minimal required fields (no defaults section, minimal components).
    """
    test_name = "minimal_config_test"
    input_file = tests_dir / test_name / "input" / "lm_config.json"

    run(input_file, test_name, schema_file)


def test_full_config(schema_file):
    """
    Test generation of launch manager configuration
    with all parameters specified at every level (defaults, components, Run Targets,
    alive supervision, watchdog, sandbox, etc.).
    """
    test_name = "full_config_test"
    input_file = tests_dir / test_name / "input" / "lm_config.json"

    run(input_file, test_name, schema_file)


# Define the parameterized test cases
CUSTOM_VALIDATION_TEST_CASES = [
    {
        "config_filename": "fallback_run_target.json",
        "expected_pass": False,
        "expected_logs": [
            'recovery RunTarget must be set to "fallback_run_target"',
            'RunTarget name "fallback_run_target" is reserved',
        ],
        "test_id": "fallback_run_target_name",
    },
    {
        "config_filename": "reporting_and_terminated_ready_condition.json",
        "expected_pass": False,
        "expected_logs": [
            "Error: Component 'reporting_and_kterminated': application_type = 'Reporting' only currently supports a 'Running' process state as a ready condition",
        ],
        "test_id": "reporting_terminated_condition",
    },
    {
        "config_filename": "shutdown_timeout_and_terminated_ready_condition.json",
        "expected_pass": True,
        "expected_logs": [
            "Warning: Component 'shutdown_timeout_and_kterminated': 'shutdown_timeout_ms' has been configured with a Terminated ready condition. The configured 'shutdown_timeout_ms' will have no effect.",
        ],
        "test_id": "shutdown_timeout_condition",
    },
]


@pytest.mark.parametrize(
    "config_filename, expected_pass, expected_logs",
    [
        (case["config_filename"], case["expected_pass"], case["expected_logs"])
        for case in CUSTOM_VALIDATION_TEST_CASES
    ],
    ids=[case["test_id"] for case in CUSTOM_VALIDATION_TEST_CASES],
)
def test_custom_validation_failures(
    schema_file, config_filename, expected_pass, expected_logs
):
    """Test that custom validation checks correctly identify various invalid configurations."""
    test_name = "custom_validation_failures_test"
    input_file = tests_dir / test_name / "input" / config_filename
    if expected_pass:
        result = run(input_file, test_name, schema_file, compare_output=False)
        actual_output = result.stdout + "\n" + result.stderr
        for expected_log in expected_logs:
            assert expected_log in actual_output, (
                f"Expected warning/message not found: {expected_log}\n"
                f"Actual script output: {actual_output}"
            )
    else:
        try:
            run(input_file, test_name, schema_file)
            raise AssertionError(
                f"Expected validation failure for {config_filename}, but the mapping script executed successfully."
            )
        except subprocess.CalledProcessError as e:
            assert e.returncode == CUSTOM_VALIDATION_FAILURE, (
                f"Expected exit code {CUSTOM_VALIDATION_FAILURE}, got {e.returncode}"
            )
            actual_error_output = e.stderr
            for expected_log in expected_logs:
                assert expected_log in actual_error_output, (
                    f"Expected error message not found: {expected_log}\\n"
                    f"Actual error output: {actual_error_output}"
                )


def test_schema_validation_failures(schema_file):
    """
    Test that schema validation errors are correctly raised when the input configuration does not conform to the defined JSON schema.
    The input configuration contains the following issues:
    * Missing required field "initial_run_target"
    """
    test_name = "schema_validation_failure_test"
    input_file = tests_dir / test_name / "input" / "lm_config.json"

    try:
        run(input_file, test_name, schema_file)
        raise AssertionError(
            "Expected an error due to schema validation failures, but the mapping script executed successfully."
        )
    except subprocess.CalledProcessError as e:
        assert e.returncode == SCHEMA_VALIDATION_FAILURE, (
            f"Expected exit code {SCHEMA_VALIDATION_FAILURE}, got {e.returncode}"
        )
