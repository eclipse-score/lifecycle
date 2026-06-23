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

Configuration
*************

The Launch Manager reads its configuration from a FlatBuffers binary file. When building
with Bazel, the ``launch_manager_config`` rule handles the conversion from a human-readable
JSON file to the binary format automatically.

Bazel Build Rule
================

Add a ``launch_manager_config`` target to your ``BUILD`` file:

.. code-block:: starlark

   load("//:defs.bzl", "launch_manager_config")

   launch_manager_config(
       name = "my_config",
       config = ":my_config.json",
       flatbuffer_out_dir = "etc",
   )

=====================

Create a JSON configuration file that describes the components to launch, the run targets,
and the alive supervision settings.

.. dropdown:: Example configuration (lifecycle_demo_test.json)

   .. literalinclude:: ../../../../examples/demo_verification/lifecycle_demo_test.json
      :language: json

For a full description of all available fields see :doc:`configuration`.

Running the Launch Manager
**************************

Start the daemon by pointing it at the generated FlatBuffers binary with the ``-c`` option:

.. code-block:: bash

   launch_manager -c etc/my_config_gen.bin

Command-line Options
====================

.. code-block:: text

   Usage: launch_manager [-c <config>] [-h]

   Options:
     -c <config>  Path to the flatbuffer config binary.
                  Default: etc/launch_manager_config_gen.bin
     -h           Print this help and exit.

Passing an unknown option prints the usage line and exits with a non-zero exit code.