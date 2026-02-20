#!/usr/bin/env python3
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

"""
Bundle a multi-file JSON Schema into a single file using $defs.

Usage:
  python scripts/bundle_defs.py \
      --input  config/schema/s-core-launch-manager.schema.json \
      --output config/schema/s-core-launch-manager.defs.bundle.json


Notes:
- Instead of fully inlining (flattening) each $ref, this script:
    1) Collects every local file $ref (e.g. "./types/*.schema.json").
    2) Imports each referenced schema once into the top-level "$defs" (deduplicated).
    3) Rewrites $ref values to point to "#/$defs/<name>" (JSON Pointer fragments like "#/foo/bar" are preserved by converting)
        * For example: "file.json#/foo/bar" -> "#/$defs/<name>/foo/bar".
- If the imported schema itself has local file refs, those are also pulled into $defs recursively with the same logic.
- Name derivation for $defs keys: file basename without ".schema.json" (e.g., "watchdog"). Collisions are resolved by appending a numeric suffix (e.g., watchdog_2).
- To avoid base-URI confusion inside bundled defs, $id and nested $schema are stripped from the imported defs.
"""

from __future__ import annotations
import argparse
import json
from pathlib import Path
from typing import Any, Dict, Tuple

Json = Any


class DefsBundler:
    def __init__(self, input_file: Path) -> None:
        self.input_file = input_file.resolve()
        self.defs: Dict[str, Json] = {}
        self.file_to_defname: Dict[Path, str] = {}

    @staticmethod
    def _deepcopy(obj: Json) -> Json:
        return json.loads(json.dumps(obj))

    @staticmethod
    def _is_local_file_ref(ref: str) -> bool:
        if not isinstance(ref, str):
            return False
        if ref.startswith("#"):
            return False
        if "://" in ref:
            return False
        return True

    @staticmethod
    def _split_ref(ref: str) -> Tuple[str, str]:
        # Returns (file_part, fragment_pointer) where fragment_pointer is like "/a/b" (without leading '#') or ''
        if "#" in ref:
            file_part, frag = ref.split("#", 1)
            if frag.startswith("/"):
                return file_part, frag  # JSON Pointer already
            if frag.startswith("#/"):
                return file_part, frag[1:]
            # treat unknown as JSON Pointer missing leading '/'
            return file_part, "/" + frag if frag else ""
        return ref, ""

    @staticmethod
    def _derive_name_from_file(path: Path) -> str:
        name = path.stem  # e.g., "watchdog.schema"
        if name.endswith(".schema"):
            name = name[:-7]  # remove trailing ".schema"
        return name

    def _unique_def_name(self, base: str) -> str:
        if base not in self.defs:
            return base
        i = 2
        while True:
            cand = f"{base}_{i}"
            if cand not in self.defs:
                return cand
            i += 1

    def _strip_ids(self, schema: Json) -> Json:
        # Remove $id and nested $schema fields from imported defs to avoid base-URI conflicts
        if isinstance(schema, dict):
            schema = {
                k: self._strip_ids(v)
                for k, v in schema.items()
                if k not in ("$id", "$schema")
            }
        elif isinstance(schema, list):
            schema = [self._strip_ids(v) for v in schema]
        return schema

    def _register_def_from_file(self, current_file: Path, ref_path: str) -> str:
        target = (current_file.parent / ref_path).resolve()
        if target in self.file_to_defname:
            return self.file_to_defname[target]
        with open(target, "r", encoding="utf-8") as f:
            schema = json.load(f)
        name_base = self._derive_name_from_file(target)
        name = self._unique_def_name(name_base)
        cleaned = self._strip_ids(schema)
        # Before storing, rewrite refs inside this imported schema
        rewritten = self._rewrite_refs(cleaned, target)
        self.defs[name] = rewritten
        self.file_to_defname[target] = name
        return name

    def _rewrite_refs(self, node: Json, current_file: Path) -> Json:
        # Traverse node; for any local file $ref, add that file into $defs and rewrite the $ref to #/$defs/<name>/fragment
        if isinstance(node, dict):
            if "$ref" in node and isinstance(node["$ref"], str):
                ref_str = node["$ref"]
                if self._is_local_file_ref(ref_str):
                    file_part, frag = self._split_ref(ref_str)
                    defname = (
                        self._register_def_from_file(current_file, file_part)
                        if file_part
                        else None
                    )
                    if defname:
                        # Compose new JSON Pointer: #/$defs/<defname><frag>
                        pointer = f"#/$defs/{defname}{frag}"
                        return {
                            **{k: v for k, v in node.items() if k != "$ref"},
                            "$ref": pointer,
                        }
            # Otherwise, descend
            return {k: self._rewrite_refs(v, current_file) for k, v in node.items()}
        elif isinstance(node, list):
            return [self._rewrite_refs(v, current_file) for v in node]
        else:
            return node

    def bundle(self, out_path: Path) -> None:
        with open(self.input_file, "r", encoding="utf-8") as f:
            root = json.load(f)

        bundled_root = self._deepcopy(root)

        # Rewrite refs in the root
        bundled_root = self._rewrite_refs(bundled_root, self.input_file)

        # Merge with existing $defs if any
        existing_defs = (
            bundled_root.get("$defs", {}) if isinstance(bundled_root, dict) else {}
        )
        if not isinstance(existing_defs, dict):
            existing_defs = {}

        merged_defs: Dict[str, Json] = {}

        # Copy existing defs
        for k, v in existing_defs.items():
            merged_defs[k] = v

        # Add generated defs (dedupe)
        for k, v in self.defs.items():
            kk = k
            ctr = 2
            while kk in merged_defs:
                kk = f"{k}_{ctr}"
                ctr += 1
            merged_defs[kk] = v

        # ---------- force a specific order in generated schema ----------
        if isinstance(bundled_root, dict):
            new_root = {}

            # The "$schema", "type", "$id", "title", and "description" elements should be at the top of the file
            for key in ["$schema", "type", "$id", "title", "description"]:
                if key in bundled_root:
                    new_root[key] = bundled_root[key]

            # Then insert "$defs" (before "properties")
            new_root["$defs"] = merged_defs

            # Insert "properties" immediately after "$defs"
            if "properties" in bundled_root:
                new_root["properties"] = bundled_root["properties"]

            # Insert remaining keys (required, additionalProperties, etc.)
            for key, value in bundled_root.items():
                if key not in new_root:
                    new_root[key] = value

            bundled_root = new_root
        # ----------------------------------------------------------------

        out_path.parent.mkdir(parents=True, exist_ok=True)
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(bundled_root, f, indent=2, ensure_ascii=False)


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Bundle multi-file JSON Schema into one file using $defs (deduplicated)."
    )
    ap.add_argument("--input", required=True, help="Path to the top-level schema JSON")
    ap.add_argument(
        "--output", required=True, help="Path to write the bundled schema JSON"
    )
    args = ap.parse_args()

    bundler = DefsBundler(Path(args.input))
    bundler.bundle(Path(args.output))
    print(f"Bundled schema written to: {args.output}")


if __name__ == "__main__":
    main()
