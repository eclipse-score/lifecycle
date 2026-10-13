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

Feature Architecture
====================

.. document:: Lifecycle Module Architecture
   :id: doc__lifecycle_module_architecture
   :status: valid
   :safety: ASIL_B
   :security: YES
   :version: 1
   :realizes: wp__feature_arch[version==1]

Overview
--------

A brief overview of Lifecycle is described :need:`doc__lifecycle`.

Description
-----------

The concept is based on 2 major components:

* **Launch Manager**: Responsible for starting and stopping components based on
  the defined Run States and alive supervision of the started components

* **Health Monitor**: Provides process local monitoring functionalities such as
  deadline monitoring and logical program flow monitoring


Rationale Behind Architecture Decomposition
*******************************************

The decomposition of the monitoring functionality between the :term:`Launch
Manager` and the :term:`Health Monitor` is documented in
:need:`dec_rec__lifecycle__in_process_health_monitor`.

Static Architecture
-------------------

Feature Decomposition
*********************

.. feat_arc_sta:: Lifecycle Static View
   :id: feat_arc_sta__lifecycle__static_view_arch
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1
   :fulfils: feat_req__lifecycle__launch_support[version==1],
             feat_req__lifecycle__logging_support[version==1]
   :includes: logic_arc_int__lifecycle__lifecycle_if[version==1],
              logic_arc_int__lifecycle__alive_if[version==1],
              logic_arc_int__lifecycle__controlif[version==1],
              logic_arc_int__lifecycle__deadline_monitor_if[version==1],
              logic_arc_int__lifecycle__logical_monitor_if[version==1]
   :belongs_to: feat__lifecycle

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_feature(need(), needs) }}
      artifact "Configuration" as cfg

      comp__lifecycle_launch_manager --> cfg: use


.. _lifecycle_configuration_structure:

Configuration Structure
***********************

.. feat_arc_sta:: Configuration parameters static architecture
   :id: feat_arc_sta__lifecycle__cfg_params_static
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1
   :fulfils: feat_req__lifecycle__component_group_config[version==1],
             feat_req__lifecycle__run_target_support[version==1]
   :belongs_to: feat__lifecycle
   :includes: logic_arc_int__lifecycle__lifecycle_if[version==1],
              logic_arc_int__lifecycle__alive_if[version==1],
              logic_arc_int__lifecycle__controlif[version==1],
              logic_arc_int__lifecycle__deadline_monitor_if[version==1],
              logic_arc_int__lifecycle__logical_monitor_if[version==1]

   .. uml:: _assets/config_params_static.puml
      :scale: 50
      :align: center

   The Launch Manager configuration is structured around Run Target and Component configurations.
   Each Component configuration specifies how the Component is started, monitored, and recovered during system operation.
   Components can have dependencies on other Components, collectively forming a dependency graph.

   Run Targets are configured through Run Target configurations, which define dependencies on Components and other Run Targets.
   These dependencies determine which Components are activated when a Run Target is activated.

   The Launch Manager is configured with an *Initial Run Target* and *Fallback Run Target*. The *Initial Run Target* is activated when the Launch Manager is first started.
   The *Fallback Run Target* specifies the Run Target that is activated if a failure occurs that cannot be recovered.


Dynamic Architecture
--------------------

.. toctree::
   :maxdepth: 1

   components_run_targets
   run_target_activation
   startup_shutdown
   health_monitoring
   recovery_actions
   external_monitor


Logical Interfaces
------------------

The logical interfaces of the feature are defined in the `interfaces` section
of the feature documentation in the project repository:
:need:`doc__lifecycle_architecture`


Design Decisions
----------------

.. toctree::
   :maxdepth: 1

   design_decisions


