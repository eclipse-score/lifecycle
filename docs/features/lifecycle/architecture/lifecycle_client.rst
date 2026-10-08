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

:orphan:

Lifecycle Client
################

The lifecycle interface is defined here:
:need:`logic_arc_int__lifecycle__lifecycle_if`

The following use cases are supported by the Lifecycle Interface, with
different capabilities depending on the application type.

**SCORE Application State Communication**

SCORE applications implementing the Lifecycle Interface can communicate their
internal state to the :term:`Launch Manager`.
The :term:`Launch Manager` uses this state information for:

- Dependency resolution for other applications
- Recovery action decisions
- Status reporting to external state managers via the Control Interface

**Shutdown Signal Handling**

The Launch Manager sends SIGTERM signal to SCORE applications (and any POSIX processes) to request their shutdown.
The lifecycle interface sets up the necessary signal handler for an SCORE application to properly respond to shutdown requests.

Dynamic Architecture
====================

A component maintains the following internal state machine:

.. feat_arc_dyn:: Component State Machine
   :id: feat_arc_dyn__lifecycle__comp_sm_if
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :fulfils: feat_req__lifecycle__process_termination[version==1],
             feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__process_ordering[version==1],
             feat_req__lifecycle__control_commands[version==1],
             feat_req__lifecycle__conditional_startup[version==1],
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/lifecycle_component_state_machine.puml
      :scale: 50
      :align: center

Note that the state machine of the underlying process is different from the component state machine.
For example, the process may be already terminated, but the component is in `Ready` state.
This is the case if the component is configured to be ready when the underlying process terminated.

.. feat_arc_dyn:: Process State Machine
   :id: feat_arc_dyn__lifecycle__proc_sm_if
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :fulfils: feat_req__lifecycle__process_termination[version==1],
             feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__process_ordering[version==1],
             feat_req__lifecycle__conditional_startup[version==1],
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/lifecycle_process_state_machine.puml
      :scale: 50
      :align: center

SCORE applications use the Lifecycle Interface for reporting the "Running" state to signal they finished their initialization.
This transitions the process state machine from state `Starting` to state `Running`.
For Native applications which do not report their state, the process state machine advances directly to `Running` after successful start of the process.