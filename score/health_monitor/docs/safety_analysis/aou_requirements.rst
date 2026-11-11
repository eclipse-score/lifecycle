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

AoU Component Requirements
==========================

.. document:: HealthMonitor Component AoU
   :id: doc__health_monitor_comp_aou
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_comp_aou[version==1]

Component AoU
-------------

.. aou_req:: CPU Time for Health Monitor Background Thread
   :id: aou_req__health_monitor__cpu_scheduling
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall ensure that the Health Monitor background thread receives sufficient CPU time
   slices to send its alive notifications within the specified interval during system operation.

   .. note:: This AoU formalizes the mitigation of :need:`doc__health_monitor_fmea` (``HM_FMEA_001``,
      "Missing processing time"). That FMEA is an informal result and does not use the S-CORE process.
      The AoU takes effect with the approval of this pull request and is set to ``valid_inspected`` by
      the requirements inspection :need:`doc__health_monitor_req_inspection`.

.. attention::
    The above directives must be updated according to your feature requirements.

    - Add other needed requirements for your feature

.. needextend:: c.this_doc() and is_external == False
   :+tags: lifecycle, health_monitor
