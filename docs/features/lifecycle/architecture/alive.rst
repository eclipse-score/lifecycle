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

Alive
#####

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

The alive interface is defined here: :need:`logic_arc_int__lifecycle__alive_if`

The :term:`Launch Manager` starts the Alive Supervision at the exact time when the Application reports its Running state via the Lifecycle Interface.
As a consequence the Liveliness Reporting is only available to SCORE applications that also use the Lifecycle Interface.

Dynamic architecture
====================

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

   .. list-table::
      :widths: 10 90
      :header-rows: 1

      * - Sequence number
        - Description
      * - 001
        - Launch Manager starts the application process.
      * - 002/003
        - Launch Manager starts the Alive Supervision at the timestamp when the application reports the Running state.
      * - 004
        - Application sends cyclic alive notifications via the Alive API to keep the Alive Supervision successful.
      * - 005/006
        - In case the Application itself detected a failure, the supervision can be directly set to failed via the Alive API.
      * - 007
        - In case the Application does not send the alive notifications with the expected frequency, the Launch Manager detects the Alive Supervision failure.
      * - 008
        - Launch Manager initiates the configured Recovery Action for this component.


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

   .. list-table::
      :widths: 10 90
      :header-rows: 1

      * - Sequence number
        - Description
      * - 001
        - Launch Manager starts the application process.
      * - 002/003
        - Launch Manager starts the Alive Supervision at the timestamp when the application reports the Running state.
      * - 004
        - Application sends cyclic alive notifications via the Alive API to keep the Alive Supervision successful.
      * - 005/006
        - In case the Application itself detected a failure, the supervision can be directly set to failed via the Alive API.
      * - 007
        - In case the Application does not send the alive notifications with the expected frequency, the Launch Manager detects the Alive Supervision failure.
      * - 008
        - Launch Manager initiates the configured Recovery Action for this component.

