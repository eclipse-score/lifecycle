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

Run Target Activation
#####################

This interface provides control functionality for activating and managing run
targets.
It allows users to trigger execution of configured :term:`Run Targets <Run
target>` through a standardized activation mechanism.

The following use cases are supported by the `Control Interface` provided by the
:term:`Launch Manager`.

Switching between Run Targets
=============================

The :term:`Launch Manager` allows switching between different :term:`Run
Targets <Run Target>`. When a switch is requested, the :term:`Launch Manager`
evaluates the current state and the target state, determining which components
need to be started or stopped based on their dependencies.

**Activating a Run Target**

When a request to activate a Run Target is received via the `ControlInterface`,
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

The control interface is defined here:
:need:`logic_arc_int__lifecycle__controlif` The :term:`Launch Manager` provides
an interface, which allows an external State Manager application to request the
:term:`Launch Manager` to start, stop or restart applications or groups of
applications, which allows the implementation of a state management
applications to support dynamic state control.

Dynamic architecture
====================

The sequence diagrams below are based on the example dependency graph described
in :doc:`./components_run_targets`.

.. feat_arc_dyn:: Switch Run Target Scenario - Successful case
   :id: feat_arc_dyn__lifecycle__control_activate
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: YES
   :fulfils: feat_req__lifecycle__control_commands[version==1]
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/control_interface_switch_sequence.puml
      :scale: 50
      :align: center


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
