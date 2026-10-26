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

.. _component_health_monitor_requirements:

Requirements
############

.. document:: HealthMonitor Requirements
   :id: doc__health_monitor_requirements
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_feat[version==1]

Safety Concept Summary
======================

The :term:`Health Monitor` is an in-process library that supervises the
application it is linked into. It detects deviations from the configured
temporal and logical behavior and reacts by withholding the alive
notification towards the :term:`Launch Manager`.

- **Safe state:** the absence of alive notifications. The :term:`Launch Manager`
  detects the missing notification via its alive supervision and executes the
  configured :term:`Recovery Action`.
- **Fail-silent principle:** any failure, whether detected in the application
  or internal to the :term:`Health Monitor` (crash, hang, panic, starvation),
  ends in the same safe state. A fault can therefore never make the
  :term:`Health Monitor` report "alive" for a failed application.
- **Detection channel:** the :term:`Launch Manager` alive supervision is the
  independent channel that covers failures of the :term:`Health Monitor` itself.

General
=======

.. comp_req:: Configuration validation
   :id: comp_req__health_monitor__cfg_validation
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1],
                  feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall reject a configuration and not create a
   monitoring instance if any of the following holds:

   - no monitor is configured,
   - the supervisor notification cycle is not an integer multiple of the
     internal evaluation cycle.

   .. note:: Monitor specific configuration checks are specified in the
      section of the respective monitor and in
      :need:`comp_req__health_monitor__cfg_time_range`.

.. comp_req:: Time range validation
   :id: comp_req__health_monitor__cfg_time_range
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall not accept a time range whose minimum is
   greater than its maximum.

.. comp_req:: Complete monitor activation
   :id: comp_req__health_monitor__complete_activation
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1],
                  feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall refuse to start supervision unless every
   configured monitor has been acquired by the application.

   .. note:: Prevents a monitor that is configured but never fed from
      silently staying unsupervised.

.. comp_req:: Exclusive monitor ownership
   :id: comp_req__health_monitor__exclusive_owner
   :reqtype: Interface
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1],
                  feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall hand out each monitor and each deadline
   at most once, and shall return an error on any further acquisition attempt.

.. comp_req:: Cyclic evaluation
   :id: comp_req__health_monitor__cyclic_eval
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1],
                  feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   Once started, the :term:`Health Monitor` shall evaluate all monitors with
   the configured internal evaluation cycle, independently of the application
   threads that report to them.

.. comp_req:: Monotonic time base
   :id: comp_req__health_monitor__monotonic_time
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall base all time supervision on a monotonic
   clock that is not affected by wall-clock adjustments.

Checkpoint (Heartbeat) Supervision
==================================

.. comp_req:: Heartbeat configuration validation
   :id: comp_req__health_monitor__hb_cfg
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall reject a heartbeat monitor configuration if
   the internal evaluation cycle is not shorter than twice the minimum of the
   heartbeat time range.

   .. note:: Ensures that a correctly behaving application never reports more
      than one heartbeat within one internal evaluation cycle. See
      :need:`comp_req__health_monitor__hb_multiple`.

.. comp_req:: Heartbeat too late
   :id: comp_req__health_monitor__hb_too_late
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall detect a heartbeat failure if no heartbeat
   is reported before the maximum of the configured heartbeat time range has
   elapsed.

.. comp_req:: Heartbeat too early
   :id: comp_req__health_monitor__hb_too_early
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall detect a heartbeat failure if a heartbeat
   is reported before the minimum of the configured heartbeat time range has
   elapsed.

.. comp_req:: Multiple heartbeats
   :id: comp_req__health_monitor__hb_multiple
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall detect a heartbeat failure if more than one
   heartbeat is reported within one internal evaluation cycle.

   .. note:: Only the latest heartbeat of an evaluation cycle is kept, so
      additional heartbeats cannot be checked against the time range
      individually. Without this check, a later heartbeat could mask an earlier
      one that was reported too early.

Deadline Supervision
====================

.. comp_req:: Deadline missed
   :id: comp_req__health_monitor__dl_missed
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_deadline[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall detect a deadline failure if a started
   deadline is not stopped before the configured maximum time has elapsed,
   including the case where it is never stopped.

.. comp_req:: Deadline too early
   :id: comp_req__health_monitor__dl_too_early
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_deadline[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall detect a deadline failure if a started
   deadline is stopped before the configured minimum time has elapsed.

.. comp_req:: Failed deadline reuse
   :id: comp_req__health_monitor__dl_failed_reuse
   :reqtype: Interface
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_deadline[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall return an error to the caller when a
   deadline that has already failed is started again.

Logical Supervision
===================

.. comp_req:: Logic configuration validation
   :id: comp_req__health_monitor__lg_cfg
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall reject a logic monitor configuration if any
   of the following holds:

   - no state is defined,
   - an allowed transition targets a state that is not defined,
   - the initial state is not defined.

.. comp_req:: Initial logical state
   :id: comp_req__health_monitor__lg_initial
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall put every logic monitor into its configured
   initial state when the monitor is created.

.. comp_req:: Invalid transition
   :id: comp_req__health_monitor__lg_invalid_trans
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall detect a logical failure if a reported
   transition is not among the configured allowed transitions from the current
   state, and shall return an error to the caller.

.. comp_req:: Unknown state
   :id: comp_req__health_monitor__lg_unknown_state
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall detect a logical failure if a transition to
   a state that is not configured is reported, and shall return an error to the
   caller.

Error Reaction
==============

.. comp_req:: Alive notification
   :id: comp_req__health_monitor__alive_notify
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__liveliness_detection[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   While no failure has been detected, the :term:`Health Monitor` shall send an
   alive notification to the :term:`Launch Manager` once per configured
   supervisor notification cycle.

.. comp_req:: Withhold alive on failure
   :id: comp_req__health_monitor__alive_withhold
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__liveliness_detection[version==1],
                  feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1],
                  feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   On detection of any supervision failure, the :term:`Health Monitor` shall
   stop sending alive notifications to the :term:`Launch Manager`.

.. comp_req:: Failure latching
   :id: comp_req__health_monitor__failure_latch
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__liveliness_detection[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   Once a supervision failure has been detected, the :term:`Health Monitor`
   shall not resume alive notifications for the lifetime of the monitoring
   instance.

.. comp_req:: Fail-silent on internal error
   :id: comp_req__health_monitor__fail_silent
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__liveliness_detection[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   On any internal error, the :term:`Health Monitor` shall stop sending alive
   notifications to the :term:`Launch Manager`. Internal errors include an
   inconsistent monitor state, a time calculation overflow, and abnormal
   termination of the evaluation thread.

.. comp_req:: Detection latency
   :id: comp_req__health_monitor__detect_latency
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__liveliness_detection[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall detect a supervision failure no later than
   one internal evaluation cycle after the failure condition has occurred.

   .. note:: Together with the :term:`Launch Manager` alive supervision
      timeout, this bounds the fault reaction time. See
      :need:`aou_req__health_monitor__ftti_budget`.

.. comp_req:: Failure logging
   :id: comp_req__health_monitor__failure_log
   :reqtype: Functional
   :security: NO
   :safety: QM
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall log each detected supervision failure with
   the monitor tag and the failure type.

Robustness and Freedom from Interference
========================================

.. comp_req:: Static memory after start
   :id: comp_req__health_monitor__static_memory
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__liveliness_detection[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall not allocate dynamic memory after
   supervision has been started.

.. comp_req:: Bounded, non-blocking reporting
   :id: comp_req__health_monitor__nonblocking_api
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1],
                  feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The reporting operations of the :term:`Health Monitor` (heartbeat, deadline
   start and stop, state transition) shall have a bounded execution time, and 
   shall not block the calling thread.

.. comp_req:: Concurrent reporting
   :id: comp_req__health_monitor__concurrency
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1],
                  feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall keep the monitor state consistent when
   reporting and cyclic evaluation run concurrently on different threads.

.. comp_req:: Memory corruption detection
   :id: comp_req__health_monitor__mem_guard
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__liveliness_detection[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall place its monitoring data between
   inaccessible memory pages, so that a write that runs across the boundaries
   of that data is detected.

   .. note:: Measure for HM_FMEA_003. It detects bulk memory corruption only.
      Targeted corruption of the monitoring data is covered by
      :need:`aou_req__health_monitor__host_integrity`.

   .. note:: Further measure like MTA will be investigated in the future.

Language Bindings
=================

.. comp_req:: Rust and C++ API
   :id: comp_req__health_monitor__lang_api
   :reqtype: Interface
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1],
                  feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall provide a Rust API and a C++ API with
   equivalent supervision semantics.

Placeholder
===========

.. comp_req:: Dummy Component
   :id: comp_req__health_monitor__dummy
   :reqtype: Process
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__launch_support
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

    .. note:: Kept until the architecture elements are relinked to the
       requirements above. See https://github.com/eclipse-score/lifecycle/issues/366.

.. needextend:: c.this_doc() and is_external == False
   :+tags: lifecycle, health_monitor
