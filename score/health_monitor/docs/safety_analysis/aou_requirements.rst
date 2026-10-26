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

.. aou_req:: Start before running
   :id: aou_req__health_monitor__start_order
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The application shall start the :term:`Health Monitor` before it reports
   the Running state to the :term:`Launch Manager`.

.. aou_req:: Launch Manager alive supervision
   :id: aou_req__health_monitor__lm_alive_cfg
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall configure the :term:`Launch Manager` alive supervision
   for every process that uses the :term:`Health Monitor`. The expected alive
   interval shall be consistent with the supervisor notification cycle of the
   :term:`Health Monitor`.

.. aou_req:: Fault tolerant time budget
   :id: aou_req__health_monitor__ftti_budget
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall show that the following sum is within the fault
   tolerant time interval of the supervised safety goal:

   - the maximum of the supervised time window,
   - one internal evaluation cycle,
   - the :term:`Launch Manager` alive timeout,

.. aou_req:: Configuration from timing analysis
   :id: aou_req__health_monitor__cfg_derivation
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall derive the time ranges and allowed transitions from the
   timing and control-flow analysis of the supervised application, and shall
   place the reporting calls where they reflect the safety-relevant behavior.

.. aou_req:: Evaluation thread scheduling
   :id: aou_req__health_monitor__thread_sched
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall configure the scheduling policy and priority of the
   :term:`Health Monitor` evaluation thread so that it meets its internal
   evaluation cycle under worst-case load.

.. aou_req:: Host application integrity
   :id: aou_req__health_monitor__host_integrity
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The process that hosts the :term:`Health Monitor` shall be developed to
   ASIL B.

   .. note:: The :term:`Health Monitor` shares the address space of the hosting
      process (see :need:`comp_req__health_monitor__mem_guard`). 

.. aou_req:: Monotonic clock provision
   :id: aou_req__health_monitor__os_clock
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The operating system shall provide a monotonic clock that meets ASIL B
   integrity and has a resolution finer than the smallest configured time
   range.

.. aou_req:: No test stubs in production
   :id: aou_req__health_monitor__no_stub
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall not enable the ``stub_supervisor_api_client`` feature
   in production builds.

.. needextend:: c.this_doc() and is_external == False
   :+tags: lifecycle, health_monitor
