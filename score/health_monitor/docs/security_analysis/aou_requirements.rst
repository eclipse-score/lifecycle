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

Health Monitor Security Assumptions of Use
==========================================

.. document:: HealthMonitor Component Security AoU
   :id: doc__health_monitor_comp_sec_aou
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :realizes: wp__requirements_comp_aou[version==1]


.. attention::
    No security AoUs are defined yet. The snippets below show the expected format.

Component AoU
-------------

.. code-block:: rst

   .. aou_req:: Next Title
      :id: aou_req__health_monitor__sec_next_title
      :reqtype: Process
      :security: YES
      :safety: QM
      :status: invalid

      The Component User shall do xyz to use the component securely

   .. aou_req:: Another Title
      :id: aou_req__health_monitor__another
      :reqtype: Process
      :security: YES
      :safety: QM
      :status: invalid
      :tags: environment

      The Component shall only be used in a xyz environment to ensure its secure and proper functioning.
