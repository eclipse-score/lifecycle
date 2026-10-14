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

.. _launch_manager_design_decisions:

Design Decisions
################

.. dec_rec:: Launch Manager Component Cancellation Points
   :id: dec_rec__lifecycle__lm_comp_cancel
   :status: accepted
   :version: 1
   :context: Architecture
   :decision: The waiting for Ready Condition is not abortable
   :consequences: Component either become Ready or time out
   :affects: comp__lifecycle_launch_manager

The activation and deactivation of a component must not be aborted when cancelling on ongoing Run Target transition.

Rationale:
The component should be left in a well defined state at all times. Either the component finished activation and is Ready or finished shutdown and becomes deactivated.

Considered alternatives:
* Simply aborting the activation / deactivation would leave the component in an undefined state.
* Aborting the activation / deactivation and (forcefully) terminating the underlying process may have unintended consequences:
  * Terminating during activation: The process may not yet have registered its own signal handler to react to SIGTERM
  * Terminating during deactivation: The process may be saving data to storage during shutdown, killing it forcefully may leave such storage corrupted (Note: During deactivation the process already received a SIGTERM signal first).

Context
-------

The Launch Manager is activating and deactivating components as part of a Run Target transition.
A new (forced) transition request may be received at any time that can cancel a currently ongoing transition.
Even though a transition in cancelled, each component must be left in a well defined state before starting a new transition.

Consequences
------------

In the worst case, the cancellation of a transition and initiation of a new transition will take until:
* the largest ready timeout + shutdown timeout of any currently activating component
* the largest shutdown timeout of any currently deactivating component