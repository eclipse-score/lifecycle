# *******************************************************************************
# Copyright (c) 2024 Contributors to the Eclipse Foundation
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

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
from pathlib import Path

from docutils import nodes
from docutils.parsers.rst import Directive

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "Module Template Project"
project_url = "https://eclipse-score.github.io/module_template/"
project_prefix = "MODULE_TEMPLATE_"
author = "S-CORE"
version = "0.1"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration


extensions = [
    "sphinx_design",
    "sphinx_needs",
    "sphinxcontrib.plantuml",
    "score_plantuml",
    "score_metamodel",
    "score_draw_uml_funcs",
    "score_source_code_linker",
    "score_layout",
]

exclude_patterns = [
    # The following entries are not required when building the documentation via 'bazel
    # build //docs:docs', as that command runs in a sandboxed environment. However, when
    # building the documentation via 'bazel run //docs:incremental' or esbonio, these
    # entries are required to prevent the build from failing.
    "bazel-*",
    ".venv_docs",
]

templates_path = ["templates"]

# Enable numref
numfig = True

class DisplayTestLogs(Directive):
    """Find and display the raw content of all test.log files."""

    def run(self):
        env = self.state.document.settings.env
        ws_root = Path(env.app.srcdir).parent

        result_nodes = []
        for dirname in ["bazel-testlogs", "tests-report"]:
            candidate = ws_root / dirname
            if not candidate.is_dir():
                continue
            for root, _, files in sorted(os.walk(candidate)):
                if "test.log" in files:
                    log_path = Path(root) / "test.log"
                    rel_path = log_path.relative_to(ws_root)

                    title = nodes.rubric(text=str(rel_path))
                    result_nodes.append(title)

                    try:
                        content = log_path.read_text(encoding="utf-8")
                    except Exception as e:
                        content = f"Error reading file: {e}"

                    code = nodes.literal_block(content, content)
                    code["language"] = "text"
                    code["source"] = str(rel_path)
                    result_nodes.append(code)

        if not result_nodes:
            para = nodes.paragraph(
                text="No test.log files found in bazel-testlogs or tests-report."
            )
            result_nodes.append(para)

        return result_nodes


def setup(app):
    app.add_directive("display-test-logs", DisplayTestLogs)