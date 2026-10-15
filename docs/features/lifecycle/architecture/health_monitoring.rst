..
   # *******************************************************************************
   # Copyright (c) 2024 Contributors to the Eclipse Foundation
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

Health Monitoring
#################

Alive Supervision
=================

The Alive Interface provides a basic watchdog functionality interface that
delivers essential monitoring capabilities for system health and responsiveness
tracking.
It implements core watchdog operations including heartbeat signals to ensure
reliable operation and automatic recovery from unresponsive states.

**SCORE Application Liveliness Reporting**

SCORE applications can periodically signal their liveliness to the
:term:`Launch Manager` through the Alive Interface.
This mechanism allows the :term:`Launch Manager` to:

- Detect application failures or hangs
- Trigger recovery actions when liveliness is lost
- Maintain accurate process health status

The liveliness mechanism includes:

- Configurable heartbeat intervals per application
- Timeout detection and failure handling

The corresponding configuration parameters are shown as `AliveSupervision` in
:ref:`Configuration Structure <lifecycle_configuration_structure>`.

The alive interface is defined here: :need:`logic_arc_int__lifecycle__alive_if`

The :term:`Launch Manager` starts the Alive Supervision at the exact time when the Application reports its Running state via the Lifecycle Interface.
As a consequence the Liveliness Reporting is only available to SCORE applications that also use the Lifecycle Interface.

Dynamic architecture
--------------------

.. feat_arc_dyn:: Alive Monitoring Start
   :id: feat_arc_dyn__lifecycle__alive_monitor_start
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :fulfils: feat_req__lifecycle__liveliness_detection[version==1]
   :includes:
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/alive_monitoring_start.puml
      :scale: 50
      :align: center


.. feat_arc_dyn:: Alive Monitoring Stop
   :id: feat_arc_dyn__lifecycle__alive_monitor_stop
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :fulfils: feat_req__lifecycle__liveliness_detection[version==1]
   :includes:
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/alive_monitoring_stop.puml
      :scale: 50
      :align: center


In-Process Supervision
======================

The :term:`Health Monitor` is a library, that together with the :term:`Launch Manager` provide a way to monitor
the application health in similar fashion as the AUTOSAR `Platform Health Manager` (PHM).

The main features of the :term:`Health Monitor` are the following monitoring functions:

- Checkpoint supervision
  - Periodic monitoring of checkpoints, which must fit the pre-configured expected number of notifications in the given interval (not too many, not too few)
  - Protecting from running checks too often or too rarely
- Deadline supervision
  - Timing requirement between two checkpoints.
- Logical
  - Specifies in which order two or more checkpoints must be called

The :term:`Health Monitor` itself is monitored via the :term:`Launch Manager` with via alive supervision only.


Error Reactions
---------------

- When the :term:`Health Monitor` detects a failed supervision, it shall stop triggering alive notifications to the :term:`Launch Manager`.
- Additionally, when the error occurs, the :term:`Health Monitor` triggers a failure notification to the :term:`Launch Manager` to reduce the time
  to react on the error. This obviously will only work if the :term:`Health Monitor` is still working and correctly scheduled. Thus the
  worst case reaction time calculations must be made on the monitoring rules specified in the :term:`Launch Manager` for the monitored application.


Deadline Monitor API
--------------------

Interface
^^^^^^^^^

The deadline monitor interface is defined here: :need:`logic_arc_int__lifecycle__deadline_monitor_if`


Dynamic Architecture
^^^^^^^^^^^^^^^^^^^^

.. feat_arc_dyn:: Application health monitoring
   :id: feat_arc_dyn__lifecycle__app_health_mon
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :fulfils: feat_req__lifecycle__liveliness_detection[version==1],
             feat_req__lifecycle__hm_checkpoint[version==1],
             feat_req__lifecycle__hm_deadline[version==1]
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/application_health_monitoring_dynamic.puml
      :scale: 50
      :align: center

   The most important interactions are the following:

   .. list-table::
      :widths: 10 90
      :header-rows: 1

      * - Sequence number
        - Description
      * - 001
        - :term:`Launch Manager` configuration for the alive monitoring of the `Monitored application` is parsed. This contains for example, what is the expected interval of alive notifications, how long grace period is given before failing to a missed (never received) alive notification etc.
      * - 002
        - Start the startup grace period timer to allow the application to startup, before timing out to a missed alive notification
      * - 003
        - The `Monitored application` is started. (To simplify, no startup checks drawn here)
      * - 004
        - The `Monitored application` instantiate and configure the HealthMonitor
      * - 006
        - Cyclic reporting aliveness to the monitor.
      * - 007
        - HealthMonitor waking up and checking if the checkpoint(s) have been called
      * - 008
        - Report aliveness to the LM's application specific supervision, observing the health of the HealthMonitor itself
      * - 009
        - Checkpoint sent, but not on time
      * - 010
        - Wake up and check if the checkpoint(s) have been triggered. In this case it was not, and thus actions 011 and 012 are triggered.
      * - 011
        - Trigger a failure event to the Launch Manager. This event allows the monitor react faster than waiting for the timeout to expire.
      * - 012
        - Additionally, triggering alive must be stopped


.. feat_arc_dyn:: Logical control flow monitoring
   :id: feat_arc_dyn__lifecycle__app_ctrl_flow_mon
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :fulfils: feat_req__lifecycle__liveliness_detection[version==1],
             feat_req__lifecycle__hm_checkpoint[version==1],
             feat_req__lifecycle__hm_logical[version==1]
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/logical_sup.puml
      :scale: 50
      :align: center
