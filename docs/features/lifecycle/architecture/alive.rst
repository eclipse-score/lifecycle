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


Dynamic architecture
====================

.. feat_arc_dyn:: Alive Monitoring
   :id: feat_arc_dyn__lifecycle__alive_monitor
   :security: YES
   :status: valid
   :version: 1
   :safety: ASIL_B
   :fulfils: feat_req__lifecycle__liveliness_detection[version==1]
   :includes:
   :belongs_to: feat__lifecycle[version==1]

   .. uml:: _assets/alive_monitoring_dynamic.puml
      :scale: 50
      :align: center


