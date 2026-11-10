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

.. _user_manual:

Lifecycle User Manual
=====================

.. document:: Lifecycle User Manual
   :id: doc__lifecycle_user_manual
   :status: draft
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__module_user_manual[version==1]


Overview
========

This user manual provides comprehensive guidance for using the Lifecycle module from an end customer perspective.
It covers installation, configuration, basic usage, and best practices for integrating this module
into your project.

The detailed product documentation of the Launch Manager (concepts, configuration, integration, examples and
known limitations) can be found in the :ref:`Launch Manager user guide <lm_user_guide>`.

For build and test of the module itself, please refer to the :ref:`quick-start-building-testing`.

Environment Needs
=================

Basic needed software environment for the module development and usage:

* **C++**: C++17
* **Rust**: toolchain as defined in ``rust-toolchain.toml``
* **Build System**: Bazel as defined in ``.bazelversion``
* **Operating Systems**: Linux, QNX

Dependencies
------------

[List key external dependencies, licenses, and version requirements]

**Example:**

* Standard library (STL/Core)
* [Other required libraries]

See also MODULE.bazel files for more details on dependencies.

Module Configuration Details
=============================

The configuration of the Launch Manager, including the configuration schema and the default values,
is described in :ref:`lm_config`.

Configuration Effects
---------------------

<Explain how the configuration settings affect the module's behavior, performance, and integration with other components. Include examples of typical configurations and their outcomes.>


Examples
========

Configuration examples are described in :ref:`lm_config_examples`.
Example applications can be found in the ``examples/`` directory of the repository.

API Reference
=============

For complete and detailed API documentation and descriptions, refer to the API documentation in the ``api_description/`` directory.

Performance Considerations
==========================

This section covers performance characteristics, optimization strategies, and resource requirements.
Refer to the ``performance/`` directory for detailed performance guides and benchmarks.

Integration Guidelines
======================

Integrating with Your Project
------------------------------

1. Add the module to your Bazel workspace:

   .. code-block:: python

      # In your MODULE.bazel
      bazel_dep(name = "score_lifecycle", version = "<version>")

2. Generate the Launch Manager configuration and integrate it into your image as described in :ref:`lm_integration`.


Version History, Compatibility, and Troubleshooting
===================================================

For comprehensive information on the following topics, refer to :ref:`release_notes`:

* Version history and changes
* Compatibility notes and upgrade instructions
* Known issues and limitations
* Troubleshooting tips and solutions
* Security vulnerabilities (CVEs)

Safety and Security
===================

**Safety-Critical Usage**: If you are using this module in a safety-critical context,
please refer to :doc:`safety_manual` for detailed safety requirements and guidelines.

**Security Considerations**: For information about security aspects and requirements,
please refer to :doc:`security_manual`.

License
=======

This module is licensed under the Apache License Version 2.0.
See the LICENSE file in the repository for full license text.

Feedback and Contributions
==========================

Your feedback and contributions are welcome! Please report issues or suggestions through the
project's issue tracker or contribute directly to the repository.
