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

.. document:: lifecycle Release Note v0.11.0
   :id: doc__lifecycle_release_note_v0_11_0
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

- **Activation Queue**: Non-forced activation requests are now queued, allowing multiple requests in quick succession to be accepted
  (See `#643 <https://github.com/eclipse-score/lifecycle/pull/643>`_).

Improvements
~~~~~~~~~~~~

- New example application `cpp_alive_supervised_app` demonstrating alive supervision from C++.
- Refactored the Component abstraction in the Process Group Manager.
- Process start on QNX now uses `posix_spawn`, Linux continues using `fork` and `execve`
- Process arguments (`argv`) are built once when the configuration is loaded instead of at each process start.
- Launch Manager worker threads use an explicitly configured stack size.
- The shutdown transition uses the configured Off timeout.
- Extended unit and integration test coverage (activation queue, alive supervision, start/stop order, lazy evaluation, logging).

Bug Fixes
~~~~~~~~~

- Fixed signals sent to a child process between `fork` and `exec` being lost.
- Fixed a hang/crash during shutdown caused by uncompleted transition slots.
- Fixed a hang when transitioning to Off while the graph was stuck aborting.
- Fixed the upper limit of `scheduling_priority` in the configuration schema (now 255 instead of 99).
- Fixed missing includes and Bazel target dependencies, and a QNX compiler warning.

Other Changes by Label
~~~~~~~~~~~~~~~~~~~~~~

- Requirements: updated activation queue requirements, replaced the consistent dependencies requirement, removed the file descriptor requirement,
  added `valid_from` to all requirements and moved some requirements to unplanned.
- Documentation: module maturity set to experimental, safety plan module roles assigned, code documentation cleaned up.
- CI: build, test and coverage use the shared Bazel workflow.

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

- None

*For any questions or support, please contact the* `Project Team <https://github.com/orgs/eclipse-score/projects/33>`_ *or raise an issue/discussion.*
