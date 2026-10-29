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

.. document:: lifecycle Release Note v0.9.0
   :id: doc__lifecycle_release_note_v0_9_0
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__module_sw_release_note[version==1]
   :tags:


Release Note v0.9.0
===================

Overview
--------

This document provides an overview of the changes, improvements, and bug fixes included in the software module release version named above
as compared to the module's origin release (which is usually the previous release).

Disclaimer
----------

This release note does not "release for production", as it does not come with a safety argumentation and a performed safety assessment.
The work products compiled in the safety package are created with care according to a process satisfying standards, but the as the project,
being a non-profit and open source organization, can not take over any liability for its content.

Changes to the Module
---------------------

New Features
~~~~~~~~~~~~

 - None
 
Improvements
~~~~~~~~~~~~

- Configuration option `deployment_config.ready_recovery_action` is now deprecated to avoid confusion btw. `ready_recovery_action` and `recovery_action`.
  Instead use the new configuration parameters `deployment_config.activation_retry_attempts` and `deployment_config.activation_retry_delay_ms`. Note that
  `deployment_config.activation_retry_delay_ms` is not yet implemented and only a placeholder.

Bug Fixes
~~~~~~~~~

- None

Other Changes by Label
~~~~~~~~~~~~~~~~~~~~~~

- None

Compatibility
~~~~~~~~~~~~~

- The following platforms are supported using the `bazel_cpp_toolchains <https://github.com/eclipse-score/bazel_cpp_toolchains>`_:

  - `x86_64-unknown-linux-gnu`
  - `aarch64-unknown-linux-gnu`
  - `x86_64-unknown-nto-qnx800`
  - `aarch64-unknown-nto-qnx800`

Performed Verification
~~~~~~~~~~~~~~~~~~~~~~

- Build on all supported platforms
- Unit test execution on all supported platforms

Known Issues
~~~~~~~~~~~~

- None

Known Vulnerabilities
~~~~~~~~~~~~~~~~~~~~~

- None

Upgrade Instructions
~~~~~~~~~~~~~~~~~~~~

- Configuration option `ready_recovery_action` is deprecated and will be removed with the next release. Replace configuration option `deployment_config.ready_recovery_action.restart.number_of_attempts` with `deployment_config.activation_retry_attempts` and `deployment_config.ready_recovery_action.restart.delay_before_restart_ms` with `deployment_config.activation_retry_delay_ms`.

*For any questions or support, please contact the* `Project Team <https://github.com/orgs/eclipse-score/projects/33>`_ *or raise an issue/discussion.*
