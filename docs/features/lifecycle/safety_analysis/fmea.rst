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

FMEA (Failure Modes and Effects Analysis)
=========================================

.. document:: Lifecycle FMEA
   :id: doc__lifecycle_fmea
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__feature_fmea[version==1]

The FMEA for the feature Lifecycle is performed. It covers the failure initiators of the feature
architecture: the messages between the :term:`Launch Manager`, the :term:`Health Monitor` and the
applications, the configuration limits, and the execution of both elements. The component-level
analysis of the Health Monitor is in :need:`doc__health_monitor_fmea`; this document refers to it
rather than repeating it.

The document is a draft. Each failure mode below is either not applicable, with the reason, or
applicable, with a failure effect, a detection, and a mitigation. Where no mitigation exists yet,
``sufficient`` is ``no`` and the entry names the open point. The status is ``draft`` until the
open points are closed and the approval of this document is given.

Failure Mode List
-----------------

.. list-table:: Fault models for the sequence diagrams of the feature
    :header-rows: 1
    :widths: 10,20,10,60

    * - ID
      - Failure Mode
      - Applicability
      - Rationale
    * - MF_01_01
      - message is not received (is a subset/more precise description of MF_01_05)
      - yes
      - The alive notifications of the applications and of the Health Monitor are the input of the
        supervision. A missing notification leads to a supervision failure, see FEAT_FMEA_001.
    * - MF_01_02
      - message received too late (only relevant if delay is a realistic fault)
      - yes
      - Checkpoints and alive notifications have deadlines. A late message is a supervision failure
        of the deadline and checkpoint supervision, see FEAT_FMEA_002.
    * - MF_01_03
      - message received too early (usually not a problem)
      - yes
      - Checkpoint supervision rejects more notifications than configured in the interval, so an early
        message is detected and not ignored, see FEAT_FMEA_002.
    * - MF_01_04
      - message not received correctly by all recipients (different messages or messages partly lost). Only relevant if the same message goes to multiple recipients.
      - no
      - Each alive notification, checkpoint and state change of the feature has exactly one recipient:
        the Launch Manager for alive notifications, the addressed process for lifecycle and control
        messages. Confirm this against the sequence diagrams during the review.
    * - MF_01_05
      - message is corrupted
      - yes
      - Monitoring data is exchanged through shared memory between the Health Monitor and the
        Launch Manager. A corrupted message can hide a failure or cause a false one, see FEAT_FMEA_003.
    * - MF_01_06
      - message is not sent
      - yes
      - A missing failure notification or alive notification is the same supervision failure as in
        MF_01_01, see FEAT_FMEA_001.
    * - MF_01_07
      - message is unintentionally sent
      - yes
      - A spurious failure notification or a spurious state change leads to an unintended recovery
        action of the Launch Manager, see FEAT_FMEA_003.
    * - CO_01_01
      - minimum constraint boundary is violated
      - yes
      - The intervals, grace periods and checkpoint counts are configured. A value below the
        minimum produces false supervision failures, see FEAT_FMEA_004.
    * - CO_01_02
      - maximum constraint boundary is violated
      - yes
      - The number of processes, run targets and checkpoints is configured and limited. A value above
        the limit exhausts resources, see FEAT_FMEA_004.
    * - EX_01_01
      - Process calculates wrong result(s) (is a subset/more precise description of MF_01_05 or MF_01_04). This failure mode is related to the analysis if e.g. internal safety mechanisms are required (level 2 function, plausibility check of the output, …) because of the size / complexity of the feature.
      - yes
      - The Launch Manager computes the run target transitions and the recovery actions from the
        configuration and the supervision results. A wrong result changes which components run, see
        FEAT_FMEA_006.
    * - EX_01_02
      - processing too slow (only relevant if timing is considered)
      - yes
      - The reaction time from a detected failure to the recovery action is bounded by the processing
        time. The worst-case reaction time is part of the Health Monitor design, see FEAT_FMEA_002.
    * - EX_01_03
      - processing too fast (only relevant if timing is considered)
      - yes
      - Checks that run too often are not allowed by the Health Monitor, see FEAT_FMEA_002.
    * - EX_01_04
      - loss of execution
      - yes
      - The Launch Manager and the Health Monitor background thread can stop executing, see FEAT_FMEA_005.
    * - EX_01_05
      - processing changes to arbitrary process
      - yes
      - The Launch Manager starts and stops processes. Starting a process other than the configured one
        is a failure of the start, see FEAT_FMEA_007.
    * - EX_01_06
      - processing is not complete (infinite loop)
      - yes
      - A blocked or looping execution of the Health Monitor or the Launch Manager stops the alive
        notifications, the same effect as EX_01_04, see FEAT_FMEA_005.

FMEA
----

For all identified applicable failure initiators, the FMEA is performed in the following table.
The mitigations are those of the design. Where a mitigation is an assumption of use, the assumption
is named; the assumptions of use of the feature are in :need:`doc__lifecycle_feat_aou`.

.. list-table:: Feature FMEA
    :header-rows: 1
    :widths: 12,18,30,30,10

    * - id
      - failure modes
      - failure effect
      - detection and mitigation
      - sufficient
    * - FEAT_FMEA_001
      - MF_01_01, MF_01_06, EX_01_06 (alive notifications missing or not sent)
      - The Launch Manager sees no alive notification of a process and does not know its state.
      - | **Detection:** the Launch Manager's alive supervision of the process, started at the Running
          state, see :need:`feat_arc_dyn__lifecycle__alive_monitor_start`.
        | **Reaction:** the supervision failure is handled by the recovery actions, see
          :need:`feat_arc_dyn__lifecycle__sup_failure`.
        | The reaction time is bounded by the configured interval and grace period. The bound is an
          assumption of use of the integrator.
      - yes, with the assumption of use for the reaction time
    * - FEAT_FMEA_002
      - MF_01_02, MF_01_03, EX_01_02, EX_01_03 (checkpoints and deadlines late, early or too frequent)
      - A checkpoint is reported too late, too early or too often, so the process is wrongly reported as
        failed or its failure is not detected in time.
      - | **Detection and mitigation:** the checkpoint, deadline and logical flow supervision of the
          Health Monitor, see :need:`feat_arc_dyn__lifecycle__app_health_mon` and
          :need:`feat_arc_dyn__lifecycle__app_ctrl_flow_mon`. The Health Monitor stops its alive
          notifications on a detected failure and sends a failure notification to the Launch Manager,
          so the reaction does not wait for the timeout.
        | The worst-case reaction time is calculated from the configured monitoring rules, see
          :need:`dec_rec__lifecycle__in_process_health_monitor`.
      - yes
    * - FEAT_FMEA_003
      - MF_01_05, MF_01_07 (corrupted or unintended message in the shared monitoring data)
      - A corrupted message hides a failure, or a spurious one triggers a recovery action of a healthy
        process. A stale process identifier in shared memory was such a defect (``UI_01_09`` in the
        dependent failure analysis, fixed by pull request #830).
      - | **Detection:** none on the message content yet. The Health Monitor's protection of its own data
          structures (``HM_FMEA_003`` in :need:`doc__health_monitor_fmea`) does not cover the messages
          of the Launch Manager.
        | **Open point:** a mitigation for the message content, owner: Janis, trigger: review of this
          document by the Safety Manager. No mitigation issue exists yet.
      - no
    * - FEAT_FMEA_004
      - CO_01_01, CO_01_02 (configured values outside their limits)
      - A configuration value outside its valid range leads to false supervision failures, or exhausts
        resources.
      - | **Detection and mitigation:** the configuration is checked when it is parsed, and the
          configuration tests (``//scripts/config_mapping/tests:lifecycle_config_tests``) cover this.
        | **Open point:** the limits of the Launch Manager JSON schema are incomplete, see
          eclipse-score/lifecycle#953. Owner: Janis, who follows up on #953; trigger: its merge.
      - no, until the limits are complete
    * - FEAT_FMEA_005
      - EX_01_04, EX_01_06 (loss or blocking of execution of the Launch Manager or of the Health Monitor)
      - The Health Monitor stops its alive notifications, or the Launch Manager stops supervising. The
        failure is not reported by the element itself.
      - | **Detection and mitigation for the Health Monitor:** the Launch Manager's alive supervision of
          the Health Monitor, and the analysis in :need:`doc__health_monitor_fmea` (``HM_FMEA_002``).
        | **Detection and mitigation for the Launch Manager:** the Launch Manager's self-supervision and
          the external watchdog, see :need:`feat_arc_dyn__lifecycle__watch`. The watchdog is outside the
          feature; its existence is an assumption of use of the integrator.
      - yes, with the assumption of use for the external watchdog
    * - FEAT_FMEA_006
      - EX_01_01 (wrong run target transition or recovery action)
      - A wrong transition or recovery action starts or stops components that should not change state.
      - | **Detection and mitigation:** the integration tests of the run target transitions and of the
          recovery actions in ``//tests/integration:integration`` (for example
          ``run_target_requests``, ``switch_run_target``, ``crash_ignores_dependents``).
        | **Open point:** the tests show behaviour, but there is no argument yet that they cover every
          transition in the design. Owner: Janis, trigger: the feature verification report.
      - no, until the argument is written
    * - FEAT_FMEA_007
      - EX_01_05 (a process other than the configured binary is started)
      - An unintended program runs with the rights of the process.
      - | **Detection and mitigation:** the start of the configured binary is checked, and the integration
          test ``//tests/integration:process_wrong_binary_failure`` reports the failure. The sandbox
          options of the configuration apply to the started process, see
          :need:`feat_req__lifecycle__sandbox_options`.
      - yes
