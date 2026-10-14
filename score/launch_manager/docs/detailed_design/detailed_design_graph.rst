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


.. Static Diagrams for Unit Interactions
   -------------------------------------
   <TODO>



Dynamic Diagrams for Unit Interactions
--------------------------------------

Graph State Machine
~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/graph_states.puml
   :align: center

The graph is the internal state machine of the Launch Manager and tracks
the process of a Run Target transition.


Component Activation
~~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/component_activation.puml
   :align: center

A component is activated on a worker thread: the process is started, registered
in the SafeProcessMap and then given until the ready timeout to reach its ready
condition. An attempt that times out is retried as long as startup attempts are
left, while a failure starting the process is final. The outcome is
reported to the graph through the component event queue.


Component Deactivation
~~~~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/component_deactivation.puml
   :align: center

Components are stopped in the reverse order of their start: a component is only
ready to be stopped once everything that depends on it is gone. The worker stops
the supervision, asks the process to terminate with SIGTERM and waits for its
exit, escalating to SIGKILL after the shutdown timeout. A deactivation therefore
has no failure outcome.


Process Termination
~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/process_termination.puml
   :align: center

Every exit of a child process is picked up by the OsHandler thread, which
resolves the process id to its component. An expected exit - a termination that
was requested, or the completion of a self terminating component - only releases
the worker that is waiting for it. Any other exit is an unexpected termination:
it leaves the active Run Target and leads to a transition to the fallback Run
Target.

Supervision Failure
~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/supervision_failure.puml
   :align: center

Alive supervision reports the failing component through the component event queue. 
The main loop reacts by requesting the fallback Run Target and cancels a running 
transition if there is one. Stopping and restarting the component is left to the 
resulting transition.


Run Target transition
~~~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/run_target_transition.puml
   :align: center

A transition runs in two phases: first every component that is running and is
not needed by the target is stopped, then every component of the target that is
not active yet is started. The graph dispatches only the components whose
predecessors have finished and never waits for them; each result that arrives in
the main loop advances the transition. A component that fails ends the
transition and moves the graph to kAborting.


Transition cancellation / preemption
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. uml:: _assets/transition_cancellation_preemption.puml
   :align: center

When cancelling an ongoing transition it is crucial to leave every component
in a well-defined state.

A cancellation is observed before a task is executed and between two startup
attempts. Everything else runs to its end: a wait for a ready condition is
bounded by the ready timeout and a termination that was already requested by the
shutdown timeout, so a cancellation settles within a bounded time.



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