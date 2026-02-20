#!/usr/bin/env python3

"""
Validate JSON instance(s) against a multi-file JSON Schema with relative $ref paths.

Usage examples:
  Validate a single file:
    python validate_config.py --schema ./schema/s-core_launch_manager.schema.json --instance ./examples/config.json

  Validate all JSON files in a directory (recursively):
    python validate_config.py --schema ./schema/s-core_launch_manager.schema.json --instances-dir ./examples

Exit codes:
  0 -> all instances are valid
  1 -> at least one instance failed validation or there was an error
"""

import argparse
import json
import sys
from pathlib import Path

try:
    from jsonschema import validators, RefResolver, FormatChecker
    from jsonschema.exceptions import RefResolutionError, SchemaError, ValidationError
except ImportError:
    print("This script requires the 'jsonschema' package. Install with:\n  pip install jsonschema", file=sys.stderr)
    sys.exit(1)


def load_json(path: Path):
    try:
        with path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        raise ValueError(f"Failed to parse JSON file '{path}': {e}") from e
    except OSError as e:
        raise ValueError(f"Failed to read file '{path}': {e}") from e


def json_pointer_path(parts):
    """
    Convert an error path into a friendly string like:
      $.topLevel.items[2].name
    """
    if not parts:
        return "$"
    s = "$"
    for p in parts:
        if isinstance(p, int):
            s += f"[{p}]"
        else:
            s += f".{p}"
    return s


def build_validator(schema_path: Path):
    schema = load_json(schema_path)

    # Choose validator based on $schema automatically (Draft-07 / 2019-09 / 2020-12, etc.)
    ValidatorClass = validators.validator_for(schema)
    # Validate the schema itself (optional but helpful)
    try:
        ValidatorClass.check_schema(schema)
    except SchemaError as e:
        raise SchemaError(f"Your schema appears invalid: {e.message}\nAt: {'/'.join(map(str, e.path))}") from e

    # Base URI for resolving relative $refs like "./types/*.schema.json"
    base_uri = schema_path.resolve().parent.as_uri() + "/"

    # RefResolver is deprecated upstream but still widely supported and reliable for local file resolution.
    resolver = RefResolver(base_uri=base_uri, referrer=schema)

    # Enable common format checks (e.g., "uri", "email", "date-time")
    format_checker = FormatChecker()

    return ValidatorClass(schema, resolver=resolver, format_checker=format_checker)


def validate_instance(validator, instance_path: Path):
    instance = load_json(instance_path)
    errors = sorted(validator.iter_errors(instance), key=lambda e: list(e.path))
    return errors


def find_json_files(root: Path):
    # Recurse and pick *.json files only
    return [p for p in root.rglob("*.json") if p.is_file()]


def main():
    parser = argparse.ArgumentParser(description="Validate JSON instance(s) against a multi-file JSON Schema.")
    parser.add_argument("--schema", required=True, type=Path, help="Path to the top-level schema (e.g., ./schema/s-core_launch_manager.schema.json)")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--instance", type=Path, help="Path to a single JSON instance to validate")
    group.add_argument("--instances-dir", type=Path, help="Path to a directory containing JSON instances (recursively)")
    parser.add_argument("--stop-on-first", action="store_true", help="Stop after the first instance with errors")
    args = parser.parse_args()

    try:
        validator = build_validator(args.schema)
    except (ValueError, SchemaError) as e:
        print(f"[Schema Error] {e}", file=sys.stderr)
        sys.exit(1)

    instance_paths = []
    if args.instance:
        instance_paths = [args.instance]
    else:
        if not args.instances_dir.exists():
            print(f"[Error] Instances directory not found: {args.instances_dir}", file=sys.stderr)
            sys.exit(1)
        instance_paths = find_json_files(args.instances_dir)
        if not instance_paths:
            print(f"[Info] No JSON files found under: {args.instances_dir}")
            sys.exit(0)

    any_failed = False
    for path in instance_paths:
        try:
            errors = validate_instance(validator, path)
        except ValueError as e:
            print(f"Error --> {path}: {e}", file=sys.stderr)
            any_failed = True
            if args.stop_on_first:
                break
            continue
        except RefResolutionError as e:
            print(f"Error --> {path}: Failed to resolve a $ref - {e}", file=sys.stderr)
            print("  Tips:")
            print("   * Ensure $ref paths like './types/...' are correct relative to the top-level schema file.")
            print("   * Make sure referenced files exist and are valid JSON schemas.")
            any_failed = True
            if args.stop_on_first:
                break
            continue

        if not errors:
            print(f"Success --> {path}: valid")
        else:
            any_failed = True
            print(f"Error --> {path}: {len(errors)} error(s)")
            for i, err in enumerate(errors, 1):
                instance_loc = json_pointer_path(err.path)
                schema_loc = "/".join(map(str, err.schema_path)) if err.schema_path else "(root)"
                print(f"  [{i}] at {instance_loc}")
                print(f"      --> {err.message}")
                print(f"      schema path: {schema_loc}")
            if args.stop_on_first:
                break

    sys.exit(1 if any_failed else 0)


if __name__ == "__main__":
    main()

