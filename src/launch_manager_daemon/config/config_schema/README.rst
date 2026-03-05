..
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


Launch Manager Configuration Schema
###################################

This folder contains the Launch Manager configuration JSON Schema. The schema defines and validates the structure of Launch Manager configuration files.

Overview
********

This project manages the Launch Manager configuration schema as a single, self-contained file. When you need to modify or extend the schema, you should directly edit `s-core_launch_manager.schema.json`.

**Project Structure:**

::

    +-- s-core_launch_manager.schema.json # The Launch Manager schema.
    +-- examples/                         # Illustrative example configuration files for the schema.
    +-- scripts/                          # Utility scripts, including a validation tool.

Quick Start
***********

For Users & Developers
======================

Whether you're validating a Launch Manager configuration against the schema, or actively developing and modifying the schema itself, here's how to interact with this project:

1.  **Locate the Schema:** The complete schema definition resides in ``s-core_launch_manager.schema.json``.
2.  **Explore Examples:** The ``examples/`` folder provides various sample Launch Manager configuration files. These are invaluable for understanding how the schema applies in practice and how to structure your own configurations.
3.  **Validate Your Configuration:** Use the provided validation script to check if your configuration file conforms to the schema:

    .. code-block:: bash

       scripts/validate.py --schema s-core_launch_manager.schema.json --instance your_config.json

Examples
********

The ``examples`` folder contains a set of sample Launch Manager configuration files. Each example demonstrates valid configurations according to the ``s-core_launch_manager.schema.json``.

**Recommendation:** If you are new to Launch Manager configurations, **start by reviewing these examples**. They offer practical insight into the expected structure, available properties, and common use cases defined by the schema.

Scripts
*******

The ``scripts`` folder houses utility scripts designed to assist with schema development.

validate.py
===========

The ``validate.py`` script is a crucial tool for verifying that any given Launch Manager configuration instance adheres to the rules defined in `s-core_launch_manager.schema.json`.

**Usage:**

To validate a configuration file (e.g., `example_conf.json` from the `examples` folder) against the schema:

.. code-block:: bash

   scripts/validate.py --schema s-core_launch_manager.schema.json --instance examples/example_conf.json
   Success --> examples/example_conf.json: valid

**When to use:**
*   **During Development:** Run this script frequently whenever you're creating or modifying a Launch Manager configuration file. It provides immediate feedback on whether your changes are valid according to the schema.
*   **Schema Development:** If you are making changes to `s-core_launch_manager.schema.json` itself, always run `validate.py` against the examples to ensure your schema changes haven't inadvertently broken existing, valid configurations.

Typical Workflow
****************

For schema developers or those creating new configurations:

1.  **Modify** the ``s-core_launch_manager.schema.json`` file (if you're updating the schema definition) or your Launch Manager configuration file.
2.  **Validate** your changes using the `scripts/validate.py` script against relevant example files or your new configuration. This iterative process helps ensure compliance and catch errors early.
