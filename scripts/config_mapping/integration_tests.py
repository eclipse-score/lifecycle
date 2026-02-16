#!/usr/bin/env python3

import subprocess
import shutil
from pathlib import Path
import filecmp
from lifecycle_config import preprocess_defaults
import json

script_dir = Path(__file__).parent
tests_dir = script_dir / "tests"
lifecycle_script = script_dir / "lifecycle_config.py"

def run(input_file : Path, test_name : str, compare_files_only=[]):
    """
    Execute the mapping script with the given input file and compare the generated output with the expected output.
    Input:
    - input_file: The path to the input JSON file for the mapping script
    - test_name: The name of the test case, which corresponds to a subdirectory in the "tests" directory containing the expected output
    """
    actual_output_dir = tests_dir / test_name / "actual_output"
    expected_output_dir = tests_dir / test_name / "expected_output"

    # Clean and create actual output directory
    if actual_output_dir.exists():
        shutil.rmtree(actual_output_dir)
    actual_output_dir.mkdir(parents=True)

    # Execute lifecycle_config.py
    cmd = [
        "python3", 
        str(lifecycle_script),
        str(input_file),
        "-o", str(actual_output_dir)
    ]

    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(f"Command executed successfully: {' '.join(cmd)}")
        print(f"Output: {result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {' '.join(cmd)}")
        print(f"Error: {e.stderr}")
        raise

    if compare_files_only:
        # Compare only specific files
        if not compare_files(actual_output_dir, expected_output_dir, compare_files_only):
            raise AssertionError("Actual output files do not match expected output files.")
    else:
        # Compare the complete directory content
        if not compare_directories(actual_output_dir, expected_output_dir):
            raise AssertionError("Actual output does not match expected output.")

def compare_directories(dir1: Path, dir2: Path) -> bool:
    """
    Compare two directories recursively. Return True if they are the same, False otherwise.
    """
    dcmp = filecmp.dircmp(dir1, dir2)
    
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

def compare_files(dir1 : Path, dir2 : Path, files : list) -> bool:
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

def test_basic():
    test_name = "basic_test"
    input_file = tests_dir / test_name / "input" / "lm_config.json"

    run(input_file, test_name="basic_test")

#def test_health_config_mapping():
#    test_name = "health_config_test"
#    input_file = tests_dir / test_name / "input" / "lm_config.json"
#    
#    run(input_file, test_name="basic_test", compare_files_only=["hm_demo.json"])
#    pass
