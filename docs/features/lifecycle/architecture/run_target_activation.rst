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

Run Target activation
#####################

The Control interface (:need:`logic_arc_int__lifecycle__controlif`), provided by the
:term:`Launch Manager`, offers control functionality for activating and managing run
targets.
It allows users to trigger execution of configured :term:`Run Targets <Run
target>` through a standardized activation mechanism.


Switching between Run Targets
=============================

The :term:`Launch Manager` allows switching between different :term:`Run
Targets <Run Target>`. When a switch is requested, the :term:`Launch Manager`
evaluates the current state and the target state, determining which components
need to be started or stopped based on their dependencies.

**Activating a Run Target**

When a request to activate a Run Target is received via the Control interface
(:need:`logic_arc_int__lifecycle__controlif`),
the :term:`Launch Manager` shall perform the following operations:

1. **Validation**: Evaluate if the conditions are correct for activating the requested Run Target:

   - The Run Target exists in the configuration
   - All dependencies for the Run Target are resolvable
   - Required resources are available

2. **Transition Logic**: Determine the transition from the current state to the target state:

   - If a different Run Target is active, perform a switch operation (stop current, start requested)
   - If the same Run Target is already active, verify its state and potentially restart failed components

3. **Execution**: Execute the transition in the correct dependency order:

   - Stop components that are not part of the new Run Target
   - Start components that are required for the new Run Target
   - Respect dependency relationships during both stop and start operations

4. **Response**: Return status to the caller:

   - Success if all components transitioned correctly
   - Failure with detailed error information if any component failed to transition

This unified approach allows external state managers to request any Run Target
activation without needing to know the current system state, as the
:term:`Launch Manager` handles the transition logic internally.

Interface
=========

The control interface is defined here: :need:`logic_arc_int__lifecycle__controlif`
The :term:`Launch Manager` provides an interface, which allows an external State Manager application to request the
:term:`Launch Manager` to switch the active Run Target (leading to start, stop or restart of applications) 
which allows the implementation of a state management application to support dynamic state control.

The Control interface (:need:`logic_arc_int__lifecycle__controlif`) provides two modes of requesting a Run Target activation:

1. **Queued Activation**

   In this mode, requests to activate a Run Target are placed in a queue and processed sequentially by the Launch Manager.
   A State Manager can issue multiple Run Target transition requests and they will be executed sequentially by the Launch Manager.

2. **Forced Activation**

   In this mode, requests to activate a Run Target are performed immediately by the Launch Manager, potentially interrupting any ongoing transitions.
   A forced activation request bypasses the queue and is executed immediately and removes any queued requests.


Dynamic architecture
====================

The sequence diagrams below are based on the example dependency graph described
in :doc:`./components_run_targets`.


.. feat_arc_dyn:: Switch Run Target Scenario - State Manager Startup
   :id: feat_arc_dyn__lifecycle__ctrl_start
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: YES
   :fulfils: feat_req__lifecycle__control_commands[version==1],
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/control_interface_startup.puml
      :scale: 50
      :align: center

   During startup the State Manager registers an activation callback with the Launch Manager to be notified about future Run Target changes.
   The State Manager can query at any time the currently active Run Target. During an ongoing transition, it will be informed that the system is in transition and no Run Target is active yet.
   After a transition is completed, the State Manager is notified via the previously registered callback about the new active Run Target.
   Queries for the currently active Run Target will now return the newly activated Run Target.

.. feat_arc_dyn:: Switch Run Target Scenario - Queued Activation
   :id: feat_arc_dyn__lifecycle__ctrl_que_act
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: YES
   :fulfils: feat_req__lifecycle__control_commands[version==1],
             feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__process_ordering[version==1],
             feat_req__lifecycle__run_target_support[version==1],
             feat_req__lifecycle__conditional_startup[version==1]
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/control_interface_queued_activation.puml
      :scale: 50
      :align: center

   The State Manager queues Run Target activations for "Running" and "Full" state.
   The Launch Manager will process these queued activations in the order they were received.
   At first the Launch Manager transitions to Run Target "Running" and subsequently to Run Target "Full".

.. feat_arc_dyn:: Switch Run Target Scenario - Forced Activation
   :id: feat_arc_dyn__lifecycle__ctrl_fced_act
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: YES
   :fulfils: feat_req__lifecycle__control_commands[version==1],
             feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__process_ordering[version==1],
             feat_req__lifecycle__run_target_support[version==1],
             feat_req__lifecycle__conditional_startup[version==1],
             feat_req__lifecycle__process_termination[version==1],
             feat_req__lifecycle__termination_dependency[version==1]
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/control_interface_forced_activation.puml
      :scale: 50
      :align: center

   The State Manager queues Run Target activations for "Running" and "Full" state.
   During activation of Run Target "Running", the State Manager sends a new forced request for Run Target "Off".
   The Launch Manager aborts the current transition and initiates the transition to Run Target "Off".
