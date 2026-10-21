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

.. aou_req:: Dummy AoU
   :id: aou_req__health_monitor__dummy
   :reqtype: Process
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1

    .. note:: This is a dummy AoU requirement. See https://github.com/eclipse-score/lifecycle/issues/366 for more information.

.. attention::
    The above directives must be updated according to your feature requirements.

    - Replace the example content by the real content for your first requirement (according to :need:`gd_guidl__req_engineering`)
    - Set the status to valid and start the review/merge process
    - Add other needed requirements for your feature

.. needextend:: c.this_doc() and is_external == False
   :+tags: lifecycle, health_monitor
