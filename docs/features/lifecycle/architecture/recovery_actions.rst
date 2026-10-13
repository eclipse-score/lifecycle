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

Recovery Actions
################

The sequence diagram below is based on the example dependency graph described
in :doc:`./components_run_targets`.

The configurable recovery actions are shown as `ComponentRecoveryAction` and
`RunTargetRecoveryAction` in
:ref:`Configuration Structure <lifecycle_configuration_structure>`.

Component crashes outside an active transition
==============================================

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
