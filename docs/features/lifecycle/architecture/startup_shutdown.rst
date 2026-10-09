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

Startup & Shutdown
##################

The sequence diagrams below are based on the example dependency graph described
in :doc:`./components_run_targets`.

Launch Manager Initial Startup
==============================

The *Initial Run Target* configuration is shown in
:ref:`Configuration Structure <lifecycle_configuration_structure>`.

.. feat_arc_dyn:: Launch Manager Startup
   :id: feat_arc_dyn__lifecycle__lcm_startup
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :belongs_to: feat__lifecycle[version==1]
   :fulfils: feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__process_ordering[version==1],
             feat_req__lifecycle__run_target_support[version==1]

   .. uml:: _assets/launch_manager_startup.puml
      :scale: 50
      :align: center

   In case of initialization failure, the Launch Manager fails to start.
   Once initialization is successful, the Launch Manager activates the configured initial Run Target.

   The continuation of this sequence, in which the Launch Manager starts the
   Components of the Run Target `Running`, is depicted in
   :need:`feat_arc_dyn__lifecycle__lcm_start`.


Launch Manager Shutdown
=======================

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
