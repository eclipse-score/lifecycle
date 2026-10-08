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

Launch manager
##############

The following describes the interaction between the
:need:`comp__lifecycle_launch_manager` and the user application.

Overview
========

The functionality of the :term:`Launch Manager` is defined by configuration
data, which spawns a directed acyclic graph (DAG) of :term:`Components
<Component>` and so called :term:`Run Targets <Run Target>`.
The :term:`Run Targets <Run Target>` are virtual nodes in the DAG and represent
:term:`Run States <Run State>` of the system.
The :term:`Launch Manager` is responsible for starting and stopping the
processes in the correct order, based on the dependencies defined in the
configuration data.

E.g. the configuration below consists of three :term:`Run Targets <Run Target>`
managing 9 components. If the user selects e.g. the :term:`Run Target` "debug"
the :term:`Launch Manager` will start the components in the following order
defined by the dependencies.

1. flash driver & eth driver
2. filesystem
3. setup filesystems
4. networking
5. ssh

.. uml:: _assets/launch_manager_target_tree.puml
   :scale: 50
   :align: center

The :need:`comp__lifecycle_launch_manager` implements the following
interfaces,for the selection of :term:`Run Target` s, starting and stopping of
components and monitoring of the processes.

Switching between Run Targets
-----------------------------

The :term:`Launch Manager` allows switching between different :term:`Run
Targets <Run Target>`. When a switch is requested, the :term:`Launch Manager`
evaluates the current state and the target state, determining which components
need to be started or stopped based on their dependencies.

When a component is started the :term:`Launch Manager` will start the
corresponding process and monitor its state via :term:`Ready Conditions <Ready
Condition>`.

:term:`Ready Conditions <Ready Condition>` are essential mechanisms that
determine when a component has successfully completed its startup phase and is
ready to fulfill its intended role in the system.
These conditions provide flexibility in defining what constitutes a "ready"
state for different types of components.
For SCORE applications, components can actively report their readiness through
the Lifecycle Interface by signaling specific states or custom conditions.
For native applications, the :term:`Launch Manager` relies on external
indicators such as process existence, file creation, network socket
availability, or successful process termination.
This dual approach ensures that both modern SCORE-aware applications and legacy
native applications can participate in the dependency management system.


Interacting with processes
--------------------------

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
applications and the :term:`Launch Manager`.

SCORE Applications
^^^^^^^^^^^^^^^^^^

**For SCORE Applications:**
- Application state reporting (started, running, stopped)
- Conditional signaling for application dependencies

See :doc:`./lifecycle_client` for the full description of the Lifecycle Interface.

Native Applications
^^^^^^^^^^^^^^^^^^^

Native applications that do not implement the Lifecycle Interface are
controlled through POSIX signals:

- **SIGTERM**: Graceful shutdown request
- **SIGKILL**: Forced termination (after timeout)

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

Example Dependency Graph
------------------------

Consider the following simplified dependency graph for the upcoming sequence diagrams.

  .. uml:: _assets/launch_manager_small_example_graph.puml
    :scale: 50
    :align: center

The Component `setup_filesystems` starts a shell script `setup_filesystems.sh` and reaches its `Ready` state when the script completed.
The Component `random` starts a binary `/bin/random` and is considered `Ready` when the file `/dev/random` exists.
The Component `app1` starts a binary `/opt/bin/app1` and is considered `Ready` when the process signaled its Running state via the Lifecycle API.

Launch Manager Initial Startup
------------------------------

.. feat_arc_dyn:: Launch Manager Startup
   :id: feat_arc_dyn__lifecycle__lcm_startup
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :belongs_to: feat__lifecycle[version==1]
   :fulfils: feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__process_ordering[version==1],
             feat_req__lifecycle__start_named_run_target[version==1]

   .. uml:: _assets/launch_manager_startup.puml
      :scale: 50
      :align: center

   In case of initialization failure, the Launch Manager fails to start.
   Once initialization is successful, the Launch Manager activates the configured initial Run Target.

   The continuation of this sequence, in which the Launch Manager starts the
   Components of the Run Target `Running`, is depicted in
   :need:`feat_arc_dyn__lifecycle__lcm_start`.


.. feat_arc_dyn:: Launch Manager Shutdown
   :id: feat_arc_dyn__lifecycle__lcm_shutdown
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :belongs_to: feat__lifecycle[version==1]
   :fulfils: feat_req__lifecycle__process_termination

   .. uml:: _assets/launch_manager_shutdown.puml

   When the Launch Manager receives a SIGTERM signal, it will initiate transition to Run Target `Off` which leads to termination of all components.
   Afterwards, Launch Manager process itself exits.
   
   The transition to the Run Target `Off`, in which the Launch Manager terminates
   the Components of the currently active Run Target, is depicted in
   :need:`feat_arc_dyn__lifecycle__lcm_term`.


Component Dependencies and Ready Conditions
-------------------------------------------

.. feat_arc_dyn:: Launch Manager - Component Dependencies
   :id: feat_arc_dyn__lifecycle__lcm_start
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :belongs_to: feat__lifecycle[version==1]
   :fulfils: feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__process_ordering[version==1],
             feat_req__lifecycle__run_target_support[version==1],
             feat_req__lifecycle__conditional_startup[version==1]

   .. uml:: _assets/launch_manager_run_target_running.puml
      :scale: 50
      :align: center

   While `setup_filesystems` and `/bin/random` can be started in parallel,  `/opt/bin/app1` can only be started once both Components are `Ready`.


Component Termination
---------------------

.. feat_arc_dyn:: Launch Manager - Component Termination
   :id: feat_arc_dyn__lifecycle__lcm_term
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :belongs_to: feat__lifecycle[version==1]
   :fulfils: feat_req__lifecycle__process_termination[version==1],
             feat_req__lifecycle__process_ordering[version==1],
             feat_req__lifecycle__termination_dependency[version==1]

   .. uml:: _assets/launch_manager_run_target_off.puml
      :scale: 50
      :align: center

   Since `app1` depends on both `setup_filesystems` and `random`, it must be terminated first before the other two components can be safely terminated.
   Afterwards, `setup_filesystems` and `random` can be terminated in parallel as they do not depend on each other.
   Note that if a component does not terminate within the configured termination timeout, it is forcefully killed.
  

Recovery Action
----------------

Component crashes outside an active transition
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. feat_arc_dyn:: Launch Manager - Crash Recovery
   :id: feat_arc_dyn__lifecycle__crash
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :belongs_to: feat__lifecycle[version==1]
   :fulfils: feat_req__lifecycle__monitor_processes[version==1],
             feat_req__lifecycle__recovery_action_support[version==1]

   .. uml:: _assets/launch_manager_random_crash.puml
      :scale: 50
      :align: center

   Note: If a Restart Recovery Action is configured then only the crashed component is restarted, not dependent components.
