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

The graph is the internal state machine of the Launch Manager and tracks
the process of a Run Target transition.
The Launch Manager main thread runs a cyclic loop and the state of the graph 
state machine defines how events are processed. 

.. uml:: _assets/graph_states.puml
   :align: center


Component Activation
~~~~~~~~~~~~~~~~~~~~

The Launch Manager main loop does not directly activate any component, but the activation
is only scheduled on a thread pool.

A component activation consists of:

* starting the process (possibly multiple times)
* registering the pid in the SafeProcessMap
* waiting for the ready condition to be fulfilled or ready timeout
* posting the activation result to the event queue

The component activation sequence is executed synchronously on a worker thread.

.. uml:: _assets/component_activation.puml
   :align: center

Component Deactivation
~~~~~~~~~~~~~~~~~~~~~~

The Launch Manager main loop does not directly deactivate any component, but the deactivation
is only scheduled on a thread pool.

A component deactivation consists of:

* stopping the process (if running)
* waiting for the process to terminate or until shutdown timeout
* escalating to SIGKILL in case the process did not terminate within the shutdown timeout
* posting the deactivation result to the event queue. Process crashes during deactivation are ignored.

The component deactivation sequence is executed synchronously on a worker thread.

.. uml:: _assets/component_deactivation.puml
   :align: center

Process Termination
~~~~~~~~~~~~~~~~~~~

The process termination may happen during component activation or deactivation but also at any
time after activation in case of a process crash or a self-terminating process.
The latter is the focus of the diagram below.

.. uml:: _assets/process_termination.puml
   :align: center


Supervision Failure
~~~~~~~~~~~~~~~~~~~

A component activates the alive supervision once the Running state is reached.
The supervision failure may be detected at any time and is reported to the component event queue.
The current failure reaction is to always request the fallback Run Target and cancel a running 
transition if there is one.

.. uml:: _assets/supervision_failure.puml
   :align: center


Run Target transition
~~~~~~~~~~~~~~~~~~~~~

A Run Target transition may be initiated either by an external request or as an error reaction (e.g. fallback_run_target).

A transition runs in two phases: first every component that is running and is
not needed by the target is stopped, then every component of the target that is
not active yet is started. The graph dispatches only the components whose
predecessors have finished and never waits for them; each result that arrives in
the main loop advances the transition. A component that fails ends the
transition and moves the graph to kAborting.

.. uml:: _assets/run_target_transition.puml
   :align: center


Transition cancellation / preemption
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When cancelling an ongoing transition it is crucial to leave every component
in a well-defined state.

The diagram below defines the point in time a component activation / deactivation
may be safely cancelled.

.. uml:: _assets/transition_cancellation_preemption.puml
   :align: center



Units within the Component
--------------------------

The Launch Manager is composed of many units. 
This detailed design focuses on the central dependency graph logic and its interaction with surrounding units.

* ProcessGroupManager: Implements the main loop of the Launch Manager
* Graph: Implements the state machine for the Run Target transition logic
* OSHandler: Waits for any child process to terminate, then forwards the process termination to process monitor
* ComponentEventQueue: Synchronizes the component events
* SafeProcessMap: Maintains a thread-safe mapping PID to component. Mapping is used to forward process termination event to the right component.
* IComponentController: Executes Component Tasks and enqueues the result to an Event Queue
* IComponent: The component interface
* Transition: Manages a RunTarget transition by determining which components are ready to start/stop