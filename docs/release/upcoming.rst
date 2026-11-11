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

.. document:: lifecycle Release Note Upcoming
   :id: doc__lifecycle_release_note_upcoming
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

- None

Improvements
~~~~~~~~~~~~

- Removes the configuration option `deployment_config.ready_recovery_action` which has been marked as deprecated since lifecycle 0.9.0 release.

Bug Fixes
~~~~~~~~~

- None

Other Changes by Label
~~~~~~~~~~~~~~~~~~~~~~

- None

Compatibility
~~~~~~~~~~~~~

- None

Performed Verification
~~~~~~~~~~~~~~~~~~~~~~

- None

Known Issues
~~~~~~~~~~~~

- None

Known Vulnerabilities
~~~~~~~~~~~~~~~~~~~~~

- None

Upgrade Instructions
~~~~~~~~~~~~~~~~~~~~

- Component and Run Target names used in `depends_on`, `initial_run_target` and `switch_run_target.run_target` may now only contain letters, digits, `_` and `-`. Configurations with other characters in these names are rejected.
- Configuration option `ready_recovery_action` is removed. Replace configuration option `deployment_config.ready_recovery_action.restart.number_of_attempts` with `deployment_config.activation_retry_attempts` and `deployment_config.ready_recovery_action.restart.delay_before_restart_ms` with `deployment_config.activation_retry_delay_ms`.


*For any questions or support, please contact the* `Project Team <https://github.com/orgs/eclipse-score/projects/33>`_ *or raise an issue/discussion.*
