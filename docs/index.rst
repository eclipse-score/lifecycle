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

Lifecycle Documentation
=======================

Portable and high-performance implementation of the Lifecycle feature for the S-CORE project,
consisting of the Launch Manager and the Health Monitor.

Feature Documentation
---------------------

The Feature documentation covers the feature-level definition of Lifecycle, including architecture and safety planning artifacts.

.. toctree::
   :maxdepth: 1

   features/index

.. needtable::
   :filter: docname is not None and docname.startswith("features/")
   :style: table
   :types: document
   :columns: title;id;safety;security;status
   :colwidths: 25,35,15,15,15
   :sort: title

Module Documentation
--------------------

The Module documentation covers the module-level view, including architecture, safety management documents, and the user manual.

.. toctree::
   :maxdepth: 1

   module/index
   verification_report/module_verification_report

.. needtable::
   :filter: docname is not None and (docname.startswith("module/") or docname.startswith("verification_report/"))
   :style: table
   :types: document
   :columns: title;id;safety;security;status
   :colwidths: 25,35,15,15,15
   :sort: title

Component Documentation
-----------------------

The Components documentation provides detailed documentation for each individual component, including requirements, architecture, and design decisions:

.. toctree::
   :maxdepth: 1

   components/index

.. needtable::
   :filter: docname is not None and docname.startswith("components/")
   :style: table
   :types: document
   :columns: title;id;safety;security;status
   :colwidths: 25,35,15,15,15
   :sort: title

Release Notes
-------------

The Release Notes document the changes, known issues and compatibility information of each module release.

.. toctree::
   :maxdepth: 1

   release/index

.. _quick-start-building-testing:

Quick Start - Building and Testing
----------------------------------

For further platforms (e.g. QNX) see the repository's README.

To build the entire module:

.. code-block:: bash

   bazel build --config=x86_64-linux //score/... //examples/...

To run all tests:

.. code-block:: bash

   bazel test --config=x86_64-linux //...

To run only the unit and component tests:

.. code-block:: bash

   bazel test --config=x86_64-linux //score/...

To run only the integration tests:

.. code-block:: bash

   bazel test --config=x86_64-linux //tests/integration/...

Module Build Configuration
--------------------------

The ``project_config.bzl`` file at the root of the module defines metadata used by Bazel macros.
This file controls build behavior and project-specific settings. It should follow the S-CORE definition.
See `S-CORE user guide for project_config.bzl <https://eclipse-score.github.io/score/main/users_guide/building_simple_application/first_score_module.html#project-config-bzl>`_ for details.

The configuration enables conditional build behavior:

* **Language-specific tools**: For C++ code, tools like ``clang-tidy`` are used; for Rust code, ``clippy`` is used
* **Safety level**: The ASIL level affects safety-related build settings and validation
* **Source code languages**: The build system optimizes for the configured languages
