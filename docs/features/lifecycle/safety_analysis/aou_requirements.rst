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

AoU Feature Requirements
========================

.. document:: Lifecycle Feature AoU
   :id: doc__lifecycle_feat_aou
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_feat_aou[version==1]

The assumptions of use below are the ones the feature's safety analysis relies on. Each one is
stated for the integrator, and each one names the design element it belongs to. They are referred to
from the feature FMEA, :need:`doc__lifecycle_fmea`.

Feature AoU
-----------

.. aou_req:: External monitor of the Launch Manager
   :id: aou_req__lifecycle__external_monitor
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall provide an external monitor that supervises the alive notifications of the
   Launch Manager and triggers a reaction when they are missing.

   .. note:: The Launch Manager cannot report its own failure, so its liveliness is supervised from
      outside, see :need:`feat_arc_dyn__lifecycle__watch`. The implementation of the external monitor
      is out of scope of the feature, as it depends on the ECU and the project.

.. aou_req:: Watchdog Proxy translation
   :id: aou_req__lifecycle__watchdog_proxy
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall provide a Watchdog Proxy that implements the HealthStatus interface and
   translates the alive events of the Launch Manager into the notifications of the external monitor.

   .. note:: The channel of the notifications (for example GPIO, UART or SPI) is project specific, see
      the external monitor of the feature architecture.

.. aou_req:: Reaction time of the monitoring rules
   :id: aou_req__lifecycle__reaction_time
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall configure the monitoring intervals, grace periods and checkpoint rules of each
   supervised application such that the worst-case reaction time stays within the reaction time that
   the item requires.

   .. note:: The worst-case reaction time is calculated from these rules, see
      :need:`dec_rec__lifecycle__in_process_health_monitor`. It is the bound used by FEAT_FMEA_001 of
      :need:`doc__lifecycle_fmea`.

.. aou_req:: Process isolation of the platform
   :id: aou_req__lifecycle__process_isolation
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall ensure, through the platform, that the processes supervised by the Launch Manager
   are isolated from each other, as assumed by :need:`aou_req__platform__process_isolation`.

   .. note:: The isolation keeps other processes from writing into the monitoring data of the Health
      Monitor. It does not cover the shared region used for the messages between the Health Monitor and
      the Launch Manager, which is shared by design. That gap is FEAT_FMEA_003 of
      :need:`doc__lifecycle_fmea`.

.. needextend:: c.this_doc() and is_external == False
   :+tags: lifecycle
