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

Lifecycle Client
################

The :term:`Launch Manager` provides interfaces for communication with launched
applications, supporting two distinct application types:

1. **SCORE Applications**: Implement the full Lifecycle Interface for
   bidirectional communication with state reporting, liveliness indication, and
   conditional signaling
2. **Native Applications**: Controlled exclusively via POSIX signals (SIGTERM,
   SIGKILL, etc.) without direct API communication

This dual approach enables the :term:`Launch Manager` to manage both legacy
native applications and SCORE-aware applications within the same system.

The Lifecycle Interface serves as the communication channel between
applications and the :term:`Launch Manager`:

**For SCORE Applications:**
- Application state reporting (started, running, stopped)
- Conditional signaling for application dependencies

**For Native Applications:**
- Process lifecycle control via POSIX signals
- Basic process monitoring (PID-based status checking)
- Exit code evaluation for failure detection

Requirements
============

Interface
=========

The lifecycle interface is defined here:
:need:`logic_arc_int__lifecycle__lifecycle_if`

The following use cases are supported by the Lifecycle Interface, with
different capabilities depending on the application type.

**SCORE Application State Communication**

SCORE applications implementing the Lifecycle Interface can communicate their
internal state to the :term:`Launch Manager`. The state information includes:

- **Started**: Application has successfully initialized and is ready to operate
- **Running**: Application is actively executing its main functionality
- **Stopped**: Application has terminated or is in the process of shutting down

The :term:`Launch Manager` uses this state information for:

- Dependency resolution for other applications
- Recovery action decisions
- Status reporting to external state managers via the Control Interface


**SCORE Application Conditional Signaling**

SCORE applications can signal custom conditions to the :term:`Launch Manager`
via the Alive Interface. This enables:

- Complex dependency management beyond simple process startup
- Coordination between interdependent applications

Custom conditions can be used by other applications as launch dependencies,
allowing for sophisticated startup orchestration.

**Native Application Control**

Native applications that do not implement the Lifecycle Interface are
controlled through POSIX signals:

- **SIGTERM**: Graceful shutdown request
- **SIGKILL**: Forced termination (after timeout)
- **SIGUSR1/SIGUSR2**: Application-specific signals (if configured)

The :term:`Launch Manager` monitors native applications through:

- Process ID (PID) tracking
- Exit code evaluation
- Resource usage monitoring via OS facilities
- Timeout-based failure detection

For native applications, the :term:`Launch Manager` provides:

- Basic lifecycle control (start/stop)
- Simple dependency management based on process existence
- Configurable startup/shutdown timeouts
- Exit code-based success/failure determination


Dynamic Architecture
====================

.. feat_arc_dyn:: Component State Machine
   :id: feat_arc_dyn__lifecycle__state_machine_if
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :fulfils: feat_req__lifecycle__process_termination[version==1],
             feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__process_ordering[version==1],
             feat_req__lifecycle__custom_cond_support[version==1],
             feat_req__lifecycle__conditional_startup[version==1],
             feat_req__lifecycle__prog_lang[version==1],
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/lifecycle_state_machine.puml
      :scale: 50
      :align: center

Requirements
============

- :need:`feat_req__lifecycle__process_termination`
- :need:`feat_req__lifecycle__launch_support`
- :need:`feat_req__lifecycle__prog_lang`
