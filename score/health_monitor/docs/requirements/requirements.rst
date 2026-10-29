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
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_comp[version==1]

Safety Concept Summary
======================

The :term:`Health Monitor` is an in-process library that supervises the
application it is linked into. It detects deviations from the configured
temporal and logical behavior and reacts by withholding the alive
notification towards the :term:`Launch Manager`.

- **Safe state:** the state the system reaches through the
  :term:`Recovery Action` of the :term:`Launch Manager`. The
  :term:`Health Monitor` triggers it by stopping its alive notifications; the
  :term:`Launch Manager` detects the missing notifications via its alive
  supervision.
- **Fail-silent principle:** any failure, whether detected in the application
  or internal to the :term:`Health Monitor` (crash, hang, panic, starvation),
  stops the alive notifications and thereby leads to the safe state. A fault
  can therefore never make the :term:`Health Monitor` report "alive" for a
  failed application.
- **Detection channel:** the :term:`Launch Manager` alive supervision is the
  independent channel that covers failures of the :term:`Health Monitor` itself.

General
=======

.. comp_req:: Configuration without monitor
   :id: comp_req__health_monitor__cfg_no_monitor
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1],
                  feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall return an error when building a
   configuration in which no monitor is configured.

   .. note:: Monitor specific configuration checks are specified in the
      section of the respective monitor and in
      :need:`comp_req__health_monitor__cfg_time_range`.

.. comp_req:: Notification cycle alignment
   :id: comp_req__health_monitor__cfg_cycle_multiple
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__liveliness_detection[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall return an error when building a
   configuration in which the supervisor notification cycle is not an integer
   multiple of the internal evaluation cycle.

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

   The :term:`Health Monitor` shall return an error when a time range is
   created whose minimum is greater than its maximum.

.. comp_req:: Time granularity
   :id: comp_req__health_monitor__time_granularity
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall handle time ranges and cycle durations with
   a granularity of one millisecond.

.. comp_req:: Monitor identification
   :id: comp_req__health_monitor__monitor_id
   :reqtype: Interface
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1],
                  feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall identify each monitor by an identifier that
   is unique within the :term:`Health Monitor` instance.

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

   The :term:`Health Monitor` shall treat a start of supervision while a
   configured monitor has not been acquired as a fatal error and terminate the
   hosting process.

   .. note:: Prevents a monitor that is configured but never fed from
      silently staying unsupervised.

.. comp_req:: Single monitor acquisition
   :id: comp_req__health_monitor__monitor_once
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_checkpoint[version==1],
                  feat_req__lifecycle__hm_deadline[version==1],
                  feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall return an error when a monitor that has
   already been handed out is requested again.

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

   Once started, and until a supervision failure has been detected, the
   :term:`Health Monitor` shall evaluate all monitors with the configured
   internal evaluation cycle, independently of the application threads that
   report to them.

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

Heartbeat (Checkpoint) Supervision
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

   The :term:`Health Monitor` shall return an error when building a heartbeat
   monitor configuration in which the internal evaluation cycle is greater than
   or equal to twice the minimum of the heartbeat time range.

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

.. comp_req:: Deadline configuration validation
   :id: comp_req__health_monitor__dl_cfg
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_deadline[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall return an error when building a deadline
   monitor configuration that contains no deadline.

.. comp_req:: Deadline identification
   :id: comp_req__health_monitor__deadline_id
   :reqtype: Interface
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_deadline[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall identify each deadline by an identifier
   that is unique within its deadline monitor.

.. comp_req:: Exclusive deadline acquisition
   :id: comp_req__health_monitor__deadline_exclusive
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_deadline[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall return an error when a deadline is
   requested while the application still holds it.

.. comp_req:: Deadline stopped late
   :id: comp_req__health_monitor__dl_late_stop
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_deadline[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall detect a deadline failure if a started
   deadline is stopped after the configured maximum time has elapsed.

.. comp_req:: Deadline not stopped
   :id: comp_req__health_monitor__dl_no_stop
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_deadline[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall detect a deadline failure if a started
   deadline is still running when the configured maximum time has elapsed.

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
   :reqtype: Functional
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

.. comp_req:: State identification
   :id: comp_req__health_monitor__state_id
   :reqtype: Interface
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall identify each state by an identifier that
   is unique within its logic monitor.

.. comp_req:: Logic configuration without state
   :id: comp_req__health_monitor__lg_cfg_no_state
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall return an error when building a logic
   monitor configuration in which no state is defined.

.. comp_req:: Undefined transition target
   :id: comp_req__health_monitor__lg_cfg_undef_target
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall return an error when building a logic
   monitor configuration in which an allowed transition targets a state that
   is not defined.

.. comp_req:: Undefined initial state
   :id: comp_req__health_monitor__lg_cfg_no_initial
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__hm_logical[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall return an error when building a logic
   monitor configuration in which the initial state is not defined.

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
   notifications to the :term:`Launch Manager`. 
   
   .. note:: Internal errors include an inconsistent monitor state, a time 
      calculation overflow, and abnormal termination of the evaluation thread.

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
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__logging_support[version==1]
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

.. comp_req:: Memory guard pages
   :id: comp_req__health_monitor__mem_guard_pages
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__liveliness_detection[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   The :term:`Health Monitor` shall place inaccessible guard pages directly
   before and after its monitoring data.

   .. note:: Measure for HM_FMEA_003.

   .. note:: Further measure like Arm Memory Tagging Extension will be
      investigated in the future.

.. comp_req:: Memory corruption detection
   :id: comp_req__health_monitor__mem_guard_detect
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__lifecycle__liveliness_detection[version==1]
   :satisfied_by: comp__health_monitor
   :status: valid
   :version: 1

   A write access to a guard page of the :term:`Health Monitor` shall
   terminate the hosting process.

   .. note:: The terminated process stops sending alive notifications, see
      :need:`comp_req__health_monitor__fail_silent`. Only bulk memory
      corruption reaches a guard page. Targeted corruption of the monitoring
      data is covered by :need:`aou_req__health_monitor__host_integrity`.

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
