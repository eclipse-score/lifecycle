..
   # *******************************************************************************
   # Copyright (c) 2025 Contributors to the Eclipse Foundation
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

This folder contains the development environment for the Launch Manager configuration JSON Schema. The schema defines and validates the structure of Launch Manager configuration files.

Overview
********

This project uses a **two-folder approach** for schema management:

- ``draft_schema/`` - Multi-file schema structure for active development
- ``published_schema/`` - Single-file schema for end-user consumption

The multi-file structure in ``draft_schema/`` makes it easier to maintain and modify the schema by organizing reusable components into separate files. When development is complete, these files are compiled into a single file in ``published_schema/`` for convenience of end users.

**Project Structure:**

::

    +-- draft_schema/          # Multi-file schema under development
    +-- published_schema/      # Single-file schema ready for use
    +-- examples/              # Sample configuration files
    +-- scripts/               # Tools for bundling and validation

Quick Start
***********

For End Users
=============

If you just want to validate your Launch Manager configuration:

1. Use the schema in ``published_schema/s-core_launch_manager.schema.json``
2. Check the ``examples/`` folder for sample configurations
3. Validate your config:

   .. code-block:: bash

      validate.py --schema published_schema/s-core_launch_manager.schema.json --instance your_config.json

For Schema Developers
======================

If you're modifying or extending the schema:

1. Edit files in ``draft_schema/``
2. Bundle your changes:

   .. code-block:: bash

      bundle.py --input draft_schema/s-core_launch_manager.schema.json --output published_schema/s-core_launch_manager.schema.json

3. Test against examples to ensure nothing broke


Examples
********

Configuration examples are provided in the ``examples`` folder, each accompanied by a brief description. **Start here** if you're new to Launch Manager configurations - these show real-world usage patterns.


Schema Development (draft_schema)
**********************************

The ``draft_schema`` folder contains the primary development work. The setup uses a multi-file structure where:

- **Reusable types** are stored in the ``types/`` subfolder
- **Top-level schema** resides in ``s-core_launch_manager.schema.json`` file

Working with $ref Paths
========================

The multi-file schema uses JSON Schema's ``$ref`` keyword to reference definitions across files. Understanding how these references work is crucial when modifying the schema.

**Key principle:** All ``$ref`` paths are relative to the location of the file containing the reference, not to any root folder.

Reference Examples
------------------

**To reference a file in a subfolder** (e.g., from ``s-core_launch_manager.schema.json`` to ``types/deployment_config.schema.json``):

.. code-block:: json

    "$ref": "./types/deployment_config.schema.json"

**To reference a file in the same folder:** (e.g., from ``types/deployment_config.schema.json`` to ``types/recovery_action.schema.json``):

.. code-block:: json

    "$ref": "./recovery_action.schema.json"

Common Pitfalls
---------------

- **Always use relative paths** starting with ``./`` or ``../``
- **Don't use absolute paths** or paths from the project root
- **Remember the current file's location** when constructing paths
- When moving files, **update all references** to and from that file

The bundling script resolves all these relative references into a single file, so the published schema doesn't need external file references.


Published Schema (published_schema)
************************************

The official, end-user consumable schema is placed in the ``published_schema`` folder. Upon completion of development, the multi-file schema from the ``draft_schema`` folder is merged into a single file and published here.

**This is the version end users should reference** in their validation tools and IDE configurations.


Scripts
*******

Utility scripts for schema development are located in the ``scripts`` folder:

bundle.py
=========

Merges the multi-file schema into a single file for end-user distribution.

**Usage:**

.. code-block:: bash

   bundle.py --input ../draft_schema/s-core_launch_manager.schema.json --output ../published_schema/s-core_launch_manager.schema.json 
   Bundled schema written to: ../published_schema/s-core_launch_manager.schema.json

**When to use:** After making changes in ``draft_schema/``, run this to create the publishable version.

validate.py
===========

Validates Launch Manager configuration instances against the schema. This script supports both single-file and multi-file schema formats.

**Validate against published schema:**

.. code-block:: bash

   validate.py --schema ../published_schema/s-core_launch_manager.schema.json --instance ../examples/example_conf.json 
   Success --> ../examples/example_conf.json: valid

**Validate against draft schema (during development):**

.. code-block:: bash

   validate.py --schema ../draft_schema/s-core_launch_manager.schema.json --instance ../examples/example_conf.json 
   Success --> ../examples/example_conf.json: valid

**When to use:** Run this frequently during development to catch errors early. Always validate examples before publishing.


Typical Workflow
****************

1. **Modify** schema files in ``draft_schema/``
2. **Validate** your changes against examples using the draft schema
3. **Bundle** the multi-file schema into a single file
4. **Validate** examples again against the published schema
