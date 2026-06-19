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

Getting Started
###############

This guide walks through creating a minimal Launch Manager configuration and running the
daemon for the first time.

Prerequisites
*************

- The ``launch_manager`` binary built and available on your target.
- Python 3 with the ``jsonschema`` package installed (required for config validation).
- The ``flatc`` FlatBuffers compiler available on your host or target.

Configuration
*************

The Launch Manager reads its configuration from a FlatBuffers binary file. The usual
workflow is:

1. Write a human-readable JSON config file.
2. Convert it to a FlatBuffers-compatible JSON using ``lifecycle_config.py``.
3. Compile the FlatBuffers JSON to a binary with ``flatc``.

Step 1 — Write the JSON config
==============================

Create a file ``my_config.json``:

.. code-block:: json

   {
       "schema_version": 1,
       "components": {
           "my_app": {
               "description": "My application",
               "component_properties": {
                   "binary_name": "my_app",
                   "application_profile": {
                       "application_type": "Native",
                       "is_self_terminating": false
                   }
               },
               "deployment_config": {
                   "bin_dir": "/opt/apps/my_app"
               }
           }
       },
       "run_targets": {
           "Startup": {
               "description": "System started",
               "depends_on": ["my_app"],
               "recovery_action": {
                   "switch_run_target": {
                       "run_target": "fallback_run_target"
                   }
               }
           }
       },
       "initial_run_target": "Startup",
       "fallback_run_target": {
           "description": "Safe shutdown",
           "transition_timeout": 1.5
       },
       "alive_supervision": {
           "evaluation_cycle": 0.5
       }
   }

Step 2 — Generate the FlatBuffers JSON
=======================================

.. note::

   When building with Bazel, steps 2 and 3 are handled automatically by the
   ``launch_manager_config`` rule (see `Bazel Integration`_). It invokes the correct
   versions of ``lifecycle_config.py`` and ``flatc`` as declared in the build graph,
   so manual invocation is only needed outside of Bazel.

.. code-block:: bash

   python3 scripts/config_mapping/lifecycle_config.py \
       my_config.json \
       --schema score/launch_manager/daemon/src/configuration/config_schema/launch_manager.schema.json \
       -o out/

This produces ``out/my_config_gen.json``.

Step 3 — Compile to a FlatBuffers binary
=========================================

.. code-block:: bash

   flatc -b -o out/ \
       score/launch_manager/daemon/src/configuration/new_lm_flatcfg.fbs \
       out/my_config_gen.json

This produces ``out/my_config_gen.bin``.

Step 4 — Run the Launch Manager
================================

By default the daemon looks for the config at ``etc/launch_manager_config_gen.bin``
relative to its working directory. Use ``-c`` to point it at a different path:

.. code-block:: bash

   launch_manager -c out/my_config_gen.bin

Command-line Options
********************

.. code-block:: text

   Usage: launch_manager [-c <config>] [-h]

   Options:
     -c <config>  Path to the flatbuffer config binary.
                  Default: etc/launch_manager_config_gen.bin
     -h           Print this help and exit.

Passing an unknown option prints the usage line and exits with a non-zero exit code.

Bazel Integration
*****************

When building with Bazel the ``launch_manager_config`` rule handles steps 2–3
automatically. It pins the versions of ``lifecycle_config.py`` and ``flatc`` that are
declared in the build graph, ensuring that the schema, the mapping script, and the
compiler are always consistent with each other. The rule writes
``launch_manager_config.bin`` into the declared output directory:

.. code-block:: python

   load("//:defs.bzl", "launch_manager_config")

   launch_manager_config(
       name = "my_config",
       config = ":my_config.json",
       flatbuffer_out_dir = "etc",
   )
