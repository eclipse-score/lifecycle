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

The sequence diagrams below are based on :ref:`the example dependency graph <lifecycle_simplified_example_graph>`.

Launch Manager initial startup
==============================

The *Initial Run Target* configuration is shown in
:ref:`Configuration Structure <lifecycle_configuration_structure>`.

.. feat_arc_dyn:: Launch Manager Startup
   :id: feat_arc_dyn__lifecycle__lcm_startup
   :security: YES
   :status: valid_inspected
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

   For detailed Run Target activation sequence refer to :doc:`./run_target_activation`.


Launch Manager shutdown
=======================

.. feat_arc_dyn:: Launch Manager Shutdown
   :id: feat_arc_dyn__lifecycle__lcm_shutdown
   :security: YES
   :status: valid_inspected
   :version: 1
   :safety: ASIL_B
   :belongs_to: feat__lifecycle[version==1]
   :fulfils: feat_req__lifecycle__process_termination[version==1]

   .. uml:: _assets/launch_manager_shutdown.puml

   When the Launch Manager receives a SIGTERM signal, it will initiate transition to Run Target `Off` which leads to termination of all components.
   Afterwards, Launch Manager process itself exits.

   For detailed Run Target activation sequence refer to :doc:`./run_target_activation`.

