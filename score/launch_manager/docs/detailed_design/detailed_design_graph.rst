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

Example: Detailed Design
========================

.. document:: Launch Manager Detailed Design
   :id: doc__launch_manager_detailed_design_graph
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__sw_implementation[version==1]


Detailed Design for Component: Launch Manager, Graph Functionality
==================================================================

Description
-----------

This detailed design focuses on the graph functionality including:

* Run Target Transitions
* Component activation & deactivation


Static Diagrams for Unit Interactions
-------------------------------------



Dynamic Diagrams for Unit Interactions
--------------------------------------

Graph State Machine
~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/graph_states.puml
   :align: center


Component Activation
~~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/component_activation.puml
   :align: center

* Include activation failure

Component Deactivation
~~~~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/component_deactivation.puml
   :align: center


Process Termination
~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/process_termination.puml
   :align: center

Supervision Failure
~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/supervision_failure.puml
   :align: center


Run Target transition
~~~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/run_target_transition.puml
   :align: center


Transition cancellation / preemption
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/transition_cancellation_preemption.puml
   :align: center

When cancelling an ongoing transition it is crucial to leave every component
in a well-defined state.

The diagram above highlights the possible cancellation points during component activation / deactivation.



Units within the Component
--------------------------

The Launch Manager is composed of many units. 
This detailed design focuses on the central dependency graph logic and its interaction with surrounding units.

* ProcessGroupManager: Implements the main loop of the Launch Manager
* Graph: Implements the state machine for the Run Target transition logic
* OSHandler: Waits for any child process to terminate, then forwards the process termination to process monitor
* ComponentEventQueue: Synchronizes the component events
* SafeProcessMap: Maintains a thread-safe mapping PID to component
* IComponentController: Executes Component Tasks and enqueues the result to an Event Queue
* IComponent: The component interface
* Transition: Manages a RunTarget transition by determining which components are ready to start/stop