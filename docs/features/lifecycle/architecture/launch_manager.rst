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

Launch manager
##############

The following describes the intaraction between the
:need:`comp__lifecycle_launch_manager` and the user application.


Requirements
============

- :need:`feat_req__lifecycle__launch_support`
- :need:`feat_req__lifecycle__process_ordering`
- :need:`feat_req__lifecycle__parallel_launch_support`
- :need:`feat_req__lifecycle__conditional_startup`
- :need:`feat_req__lifecycle__start_named_run_target`
- :need:`feat_req__lifecycle__process_termination`
- :need:`feat_req__lifecycle__terminationn_dependency`
- :need:`feat_req__lifecycle__monitor_abnormal_term`
- :need:`feat_req__lifecycle__recovery_action_support`
- :need:`feat_req__lifecycle__recov_run_target_switch`

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

1. flash driver
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
These conditions provide
flexibility in defining what constitutes a "ready" state for different types of
components.
For SCORE applications, components can actively report their readiness through
the Lifecycle Interface by signaling specific states or custom conditions.
For native applications, the :term:`Launch Manager` relies on external
indicators such as process existence, file creation, network socket
availability, or successful process termination.
This dual approach ensures that both modern SCORE-aware applications and legacy
native applications can participate in the dependency management system,
allowing the :term:`Launch Manager` to orchestrate complex startup sequences
where components depend on each other's readiness rather than just their launch
order.


.. feat_arc_dyn:: Launch Manager - Components Depends on Each Other
   :id: feat_arc_dyn__lifecycle__lcm_start
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :belongs_to: feat__lifecycle[version==1]
   :fulfils: feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__process_ordering[version==1],
             feat_req__lifecycle__start_named_run_target[version==1],
             feat_req__lifecycle__conditional_startup[version==1],

   .. uml:: _assets/launch_manager_running_dep.puml
      :scale: 50
      :align: center

   Configuration:
   Reporting App 2 depends on Reporting App 1, Reporting App 1 has a ready
   condition of begin in state running.

   .. list-table:: Sequence diagram Description
      :widths: 10 90
      :header-rows: 1

      * - Sequence number
        - Description
      * - 001
        - Launch Manager analyzes the current state of the system.
      * - 002
        - Launch Manager identifies the components that need to be started or stopped based on the current state.
      * - 003
        - Launch Manager starts the components that need to be started. In this case Reporting App 1.
      * - 004
        - Reporting App 1 is started.
      * - 005
        - Launch Manager waits for the signal from the Lifecycle API.
      * - 006
        - Reporting App 1 does its internal initialization.
      * - 007
        - Reporting App 1 signals to the Launch Manager that it has reached the ready condition using the Lifecycle API.
      * - 008
        - Launch Manager analyzes the current state of the system.
      * - 009
        - Launch Manager identifies the components that need to be started or stopped based on the current state. In this case Reporting App 2 can be started.
      * - 010
        - Launch Manager starts the components that need to be started. In this case Reporting App 2.
      * - 011
        - Reporting App 2 is started.
      * - 012
        - Launch Manager waits for the signal from the Lifecycle API.
      * - 013
        - Reporting App 2 does its internal initialization.
      * - 014
        - Reporting App 2 signals to the Launch Manager that it has reached the ready condition using the Lifecycle API.
      * - 015
        - Launch Manager analyzes the current state of the system.
      * - 016
        - Launch Manager marks the Run Target as active.


.. feat_arc_dyn:: Launch Manager - Termination Request
   :id: feat_arc_dyn__lifecycle__lcm_term
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :belongs_to: feat__lifecycle[version==1]
   :fulfils: feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__process_termination[version==1],
             feat_req__lifecycle__process_ordering[version==1],

   .. uml:: _assets/launch_manager_terminate_request.puml
      :scale: 50
      :align: center

   .. list-table:: Sequence diagram Description
      :widths: 10 90
      :header-rows: 1

      * - Sequence number
        - Description
      * - 001
        - Launch Manager analyzes the current state of the system.
      * - 002
        - Launch Manager determines the transition plan for switching the run target. In this case all components are to be terminated.
      * - 003
        - Launch Manager sends SIGTERM to the well behaving application.
      * - 004
        - The well behaving application terminates through the operating system.
      * - 005
        - Launch Manager sends SIGTERM to the badly behaving application.
      * - 006
        - The badly behaving application ignores the SIGTERM request.
      * - 007
        - Launch Manager sends SIGKILL to force termination of the badly behaving application.
      * - 008
        - The badly behaving application terminates through the operating system.

.. feat_arc_dyn:: Launch Manager - Components Depends on Termination
   :id: feat_arc_dyn__lifecycle__lcm_term_order
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :belongs_to: feat__lifecycle[version==1]
   :fulfils: feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__process_termination[version==1],
             feat_req__lifecycle__process_ordering[version==1],
             feat_req__lifecycle__terminationn_dependency[version==1]

   .. uml:: _assets/launch_manager_terminate_dep.puml
      :scale: 50
      :align: center

   Configuration:
   Reporting App 2 depends on the termination of Reporting App 1.

   .. list-table:: Sequence diagram Description
      :widths: 10 90
      :header-rows: 1

      * - Sequence number
        - Description
      * - 001
        - Launch Manager analyzes the current state of the system.
      * - 002
        - Launch Manager determines the transition plan. In this case Reporting App 1 can be started, Reporting App 2 needs to wait for Reporting App 1 to terminate.
      * - 003
        - Launch Manager starts Reporting App 1.
      * - 004
        - Reporting App 1 is started.
      * - 005
        - Launch Manager waits for the ready condition of Reporting App 1.
      * - 006
        - Reporting App 1 does its internal initialization.
      * - 007
        - Reporting App 1 signals to the Launch Manager that it has reached the ready condition using the Lifecycle API.
      * - 008
        - At some point Reporting App 1 terminates.
      * - 009
        - The operating system reports the termination of Reporting App 1 to the Launch Manager.
      * - 010
        - Launch Manager analyzes the current state of the system.
      * - 011
        - Launch Manager determines the transition plan. In this case Reporting App 2 can be started.
      * - 012
        - Launch Manager starts Reporting App 2.
      * - 013
        - Reporting App 2 is started.
      * - 014
        - Launch Manager waits for the ready condition of Reporting App 2.
      * - 015
        - Reporting App 2 does its internal initialization.
      * - 016
        - Reporting App 2 signals to the Launch Manager that it has reached the ready condition using the Lifecycle API.


.. feat_arc_dyn:: Launch Manager - Run Components in Parallel
   :id: feat_arc_dyn__lifecycle__lcm_parallel
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :belongs_to: feat__lifecycle[version==1]
   :fulfils: feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__process_ordering[version==1],
             feat_req__lifecycle__parallel_launch_support[version==1]

   .. uml:: _assets/launch_manager_parallel_dep.puml
      :scale: 50
      :align: center

   Configuration:
   Reporting App 1 and Reporting App 2 can be started independently.

   .. list-table:: Sequence diagram Description
      :widths: 10 90
      :header-rows: 1

      * - Sequence number
        - Description
      * - 001
        - Launch Manager analyzes the current state of the system.
      * - 002
        - Launch Manager determines the transition plan. In this case both Reporting App 1 and Reporting App 2 can be started in parallel.
      * - 003
        - Launch Manager starts Reporting App 1.
      * - 004
        - Launch Manager starts Reporting App 2.
      * - 005
        - Reporting App 1 is started.
      * - 006
        - Reporting App 2 is started.
      * - 007
        - Reporting App 1 does its internal initialization.
      * - 008
        - Reporting App 1 signals to the Launch Manager that it has reached the ready condition using the Lifecycle API.
      * - 009
        - Reporting App 2 does its internal initialization.
      * - 010
        - Reporting App 2 signals to the Launch Manager that it has reached the ready condition using the Lifecycle API.


.. feat_arc_dyn:: Launch Manager - Termination Request
   :id: feat_arc_dyn__lifecycle__crash
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :belongs_to: feat__lifecycle[version==1]
   :fulfils: feat_req__lifecycle__monitor_abnormal_term[version==1],
             feat_req__lifecycle__recovery_action_support[version==1],
             feat_req__lifecycle__recov_run_target_switch[version==1]

   .. uml:: _assets/launch_manager_random_crash.puml
      :scale: 50
      :align: center

   .. list-table:: Sequence diagram Description
      :widths: 10 90
      :header-rows: 1

      * - Sequence number
        - Description
      * - 001
        - The monitored process crashes unexpectedly.
      * - 002
        - The crashed process terminates through the operating system.
      * - 003
        - The operating system reports the terminated process to the Launch Manager.
      * - 004
        - Launch Manager analyzes the current state of the system.
      * - 005
        - Launch Manager determines the transition plan.\nIn this case a recovery action needs to be run,\nand the recovery action is to switch to a different Run Target.
      * - 006
        - Launch Manager switches to the configured fallback Run Target.

