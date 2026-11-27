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

.. document:: lifecycle Release Note v0.0.0
   :id: doc__lifecycle_release_note_v0_0_0
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__module_sw_release_note[version==1]
   :tags:


UPCOMING
========

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

- Component recovery action ``deployment_config.recovery_action.restart`` is now supported.
  When a component terminates unexpectedly after reaching its Ready State, only this component
  is restarted while the active Run Target stays active. The restart uses the component's
  ``activation_retry_attempts`` (and ``activation_retry_delay_ms``) settings. If the restart
  fails, the Launch Manager switches to the ``fallback_run_target``.

Improvements
~~~~~~~~~~~~

- None

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

- The ``restart`` recovery action no longer accepts the parameters ``number_of_attempts`` and
  ``delay_before_restart_ms``; it must be configured as an empty object (``"restart": {}``).
  Configure ``deployment_config.activation_retry_attempts`` and
  ``deployment_config.activation_retry_delay_ms`` instead.

*For any questions or support, please contact the* `Project Team <https://github.com/orgs/eclipse-score/projects/33>`_ *or raise an issue/discussion.*
