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

Control Client
##############

This interface provides control functionality for activating and managing run
targets.
It allows users to trigger execution of configured :term:`Run targets <Run
target>` through a standardized activation mechanism.

The following use cases are supported by the `ControlInterface` provided by the
:term:`Launch Manager`.

**Activating a Run Target**

When a request to activate a run target is received via the `ControlInterface`,
the :term:`Launch Manager` shall perform the following operations:

1. **Validation**: Evaluate if the conditions are correct for activating the requested run target:
   - The run target exists in the configuration
   - All dependencies for the run target are resolvable
   - Required resources are available

2. **Transition Logic**: Determine the transition from the current state to the target state:
   - If a different run target is active, perform a switch operation (stop current, start requested)
   - If the same run target is already active, verify its state and potentially restart failed components

3. **Execution**: Execute the transition in the correct dependency order:
   - Stop components that are not part of the new run target
   - Start components that are required for the new run target
   - Respect dependency relationships during both stop and start operations

4. **Response**: Return status to the caller:
   - Success if all components transitioned correctly
   - Failure with detailed error information if any component failed to transition

This unified approach allows external state managers to request any run target
activation without needing to know the current system state, as the
:term:`Launch Manager` handles the transition logic internally.

Interface
=========

The control interface is defined here:
:need:`logic_arc_int__lifecycle__controlif` The :term:`Launch Manager` provides
an interface, which allows an external State Manager application to request the
:term:`Launch Manager` to start, stop or restart applications or groups of
applications, which allows the implementation of a state management
applications to support dynamic state control.

Dynamic architecture
====================

.. feat_arc_dyn:: Control interface dynamic architecture activate run target
   :id: feat_arc_dyn__lifecycle__control_activate
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: YES
   :fulfils: feat_req__lifecycle__control_commands[version==1],
             feat_req__lifecycle__request_run_target_start[version==1],
             feat_req__lifecycle__switch_run_targets[version==1],
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/control_interface_start_sequence.puml
      :scale: 50
      :align: center



