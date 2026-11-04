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
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_comp_aou[version==1]

Component AoU
-------------

.. aou_req:: CPU Scheduling for Health Monitor Background Thread
   :id: aou_req__health_monitor__cpu_scheduling
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: invalid
   :version: 1

   The integrator shall configure the scheduling parameters (e.g. scheduling policy and priority)
   of the Health Monitor's background supervision thread such that it reliably receives sufficient
   CPU time slices during system operation, so that it is not starved of processing time by other
   processes or threads on the target.

   .. note:: This AoU formalizes the mitigation already described in :need:`doc__health_monitor_fmea`
      (``HM_FMEA_001``, "Missing processing time"). It is marked ``invalid`` pending formal review by
      the module's Safety Manager as part of :need:`doc__lifecycle_safety_analysis_fdr`.

.. attention::
    The above directives must be updated according to your feature requirements.

    - Add other needed requirements for your feature

.. needextend:: c.this_doc() and is_external == False
   :+tags: lifecycle, health_monitor
