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


DFA (Dependent Failure Analysis)
================================

.. document:: Lifecycle DFA
   :id: doc__lifecycle_dfa
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__feature_dfa[version==1]

.. note:: Use the content of the document to describe e.g. why a fault model is not applicable for the diagram.


The DFA for the feature Lifecycle is performed. To show evidence that all failure initiators are considered, the applicability has to be filled out in the
following tables. For all applicable failure initiators, the DFA has to be performed.

.. attention::
    **Draft status:** The applicability assessment below is a first analysis proposal prepared to
    support the module's Safety Manager, based on the already documented feature architecture (see
    :need:`doc__lifecycle_module_architecture`) and the Health Monitor FMEA (:need:`doc__health_monitor_fmea`).
    It has **not yet been formally reviewed/accepted**. The structured ``feat_saf_dfa`` entries for the
    applicable rows are intentionally not yet added, as a proper ``mitigated_by`` link requires real,
    reviewed feature/component requirements, which are not yet available for all affected areas (see
    the deviations tracked in :need:`doc__lifecycle_safety_plan`). This is planned as a follow-up once
    those requirements exist.

The "two elements" referred to below are this feature's two components, the :need:`comp__health_monitor`
(a library executing in-process within the monitored application) and the :need:`comp__lifecycle_launch_manager`
(a separate daemon process). All communication between them happens via the Alive interface
(:need:`logic_arc_int__lifecycle__alive_if`, see :need:`feat_arc_dyn__lifecycle__app_health_mon`).

Dependent Failure Initiators
----------------------------

Shared resources
^^^^^^^^^^^^^^^^

The dependent failure initiators related to shared resources are not applicable for the features. The shared resources
will be considered in the platform DFA.

Communication between the two elements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Receiving function is affected by information that is false, lost, sent multiple times, or in the wrong order etc. from the sender.

.. list-table:: DFA communication between elements
  :header-rows: 1
  :widths: 10,20,10,20

  * - ID
    - Violation cause communication between elements
    - Applicability
    - Rationale
  * - CO_01_01
    - Information passed via argument through a function call, or via writing/reading a variable being global to the two software functions (data flow)
    - no
    - Health Monitor and Launch Manager execute in separate OS processes (Health Monitor is a library
      linked into the monitored application, see :need:`doc__health_monitor_fmea`). There is no shared
      global variable or direct cross-process function call between them; all data exchange happens
      via the Alive interface, covered under CO_01_02.
  * - CO_01_02
    - Data or message corruption / repetition / loss / delay / masquerading or incorrect addressing of information
    - yes
    - Alive and Supervision-failure notifications (``ReportAlive()`` / ``ReportFailure()``, see
      :need:`feat_arc_dyn__lifecycle__app_health_mon`) are exchanged via an IPC mechanism. This class
      of fault has already manifested in practice: shared-memory pages used for this communication
      were previously mapped without read permission, causing silent data corruption on AArch64
      (`#499 <https://github.com/eclipse-score/lifecycle/issues/499>`_, fixed); a masquerading risk
      (a client mistaking an arbitrary file descriptor for the real LM shared memory) is addressed by
      `#830 <https://github.com/eclipse-score/lifecycle/pull/830>`_.
  * - CO_01_03
    - Insertion / sequence of information
    - yes
    - The protocol depends on ordering: a Supervision-failure notification must be acted upon before
      any further Alive notification is considered valid (see "Error Reactions" in
      :need:`doc__lifecycle_module_architecture`). A delayed/stale Alive notification arriving after a
      failure was already processed could mask the failure from the Launch Manager.
  * - CO_01_04
    - Corruption of information, inconsistent data
    - yes
    - Same shared-memory IPC channel as CO_01_02; see `#499 <https://github.com/eclipse-score/lifecycle/issues/499>`_
      for a concrete, previously observed instance of this failure mode on this exact communication path.
  * - CO_01_05
    - Asymmetric information sent from a sender to multiple receivers, so that not all defined receivers have the same information
    - no
    - Within this feature, the Health Monitor reports exclusively to the single Launch Manager
      instance; there is no fan-out to multiple receivers of the same notification.
  * - CO_01_06
    - Information from a sender received by only a subset of the receivers
    - no
    - Same reasoning as CO_01_05: single sender, single receiver relationship within the feature.
  * - CO_01_07
    - Blocking access to a communication channel
    - yes
    - If the IPC channel used for Alive/Failure notifications is exhausted or blocked (e.g. the
      underlying shared-memory/queue mechanism is full or the Launch Manager is not consuming it in
      time), the reporting call inside the monitored application could block, with impact on the
      monitored application itself. Needs confirmation of actual blocking vs. non-blocking semantics
      of the implementation.

Shared information inputs
^^^^^^^^^^^^^^^^^^^^^^^^^

Same information input used by multiple functions.

.. list-table:: DFA shared information inputs
  :header-rows: 1
  :widths: 10,20,10,20

  * - ID
    - Violation cause shared information inputs
    - Applicability
    - Rationale
  * - SI_01_02
    - Configuration data
    - yes
    - The Launch Manager's alive-supervision timing parameters (interval, grace period, see
      :need:`feat_arc_dyn__lifecycle__alive_monitor_start`) and the Health Monitor's internally
      configured checkpoint/wake-up intervals (configured via the application registering with the
      Health Monitor, see :need:`doc__health_monitor_fmea`) are set independently but must remain
      consistent. An inconsistent change to one without the other could cause spurious supervision
      failures or, conversely, mask a real failure.
  * - SI_01_03
    - Constants, or variables, being global to the two software functions
    - no
    - Health Monitor and Launch Manager execute in separate OS processes (see CO_01_01); no shared
      global constants or variables exist between them.
  * - SI_01_04
    - Basic software passes data (read from hardware register and converted into logical information) to two applications software functions
    - no
    - Not applicable at feature level; this is a platform/OS-level concern, consistent with the
      "Shared resources" assessment above (deferred to the platform DFA).
  * - SI_01_05
    - Data / function parameter arguments / messages delivered by software function to more than one other function
    - no
    - Same reasoning as CO_01_05/CO_01_06: the Health Monitor delivers its notifications to a single
      receiver (the Launch Manager) only.

Unintended impact
^^^^^^^^^^^^^^^^^

Unintended impacts to function due to various failures.

.. list-table:: DFA unintended impact
  :header-rows: 1
  :widths: 10,20,10,20

  * - ID
    - Violation cause unintended impact
    - Applicability
    - Rationale
  * - UI_01_01
    - Memory miss-allocation and leaks
    - no
    - Memory management of each element's own process is an intra-component concern, not a failure
      caused by the *other* element; tracked at component level (see :need:`doc__health_monitor_dfa`).
  * - UI_01_02
    - Read/Write access to memory allocated to another software element
    - yes
    - The shared-memory region used for the Alive IPC channel (see CO_01_02/CO_01_04) is, by
      construction, accessed by both elements; a previously fixed defect in its memory protection
      flags caused exactly this failure mode (`#499 <https://github.com/eclipse-score/lifecycle/issues/499>`_).
  * - UI_01_03
    - Stack/Buffer under-/overflow
    - no
    - Not a cross-element dependent failure; this is an intra-component implementation concern,
      covered by component-level FMEA/DFA and static analysis (:need:`gd_guidl__fault_models`).
  * - UI_01_04
    - Deadlocks
    - no
    - The Alive/Failure notification flow is one-directional (Health Monitor to Launch Manager) with
      no synchronous reply awaited by the sender (see :need:`feat_arc_dyn__lifecycle__app_health_mon`);
      no mutual-wait cycle between the two elements has been identified.
  * - UI_01_05
    - Livelocks
    - no
    - Same reasoning as UI_01_04: no mutual, repeating wait/retry cycle between the two elements is
      part of the documented protocol.
  * - UI_01_06
    - Blocking of execution
    - yes
    - See CO_01_07 (blocking access to the communication channel).
  * - UI_01_07
    - Incorrect allocation of execution time
    - yes
    - If the monitored application's (and thereby the Health Monitor's) scheduling parameters are
      misconfigured, the background supervision thread may not be scheduled correctly, affecting
      timely reporting to the Launch Manager. Already identified as ``HM_FMEA_001`` in
      :need:`doc__health_monitor_fmea`; a corresponding AoU mitigation is being proposed in a separate
      contribution to :need:`doc__health_monitor_comp_aou`.
  * - UI_01_08
    - Incorrect execution flow
    - no
    - No cross-element scenario beyond what is already captured under CO_01_02/CO_01_03/CO_01_04 and
      UI_01_09 has been identified; avoided here to prevent duplicate analysis.
  * - UI_01_09
    - Incorrect synchronization between software elements
    - yes
    - A previous design issue allowed a stale process identifier in shared memory to be
      misinterpreted, potentially associating data with the wrong process
      (`#830 <https://github.com/eclipse-score/lifecycle/pull/830>`_).
  * - UI_01_10
    - CPU time depletion
    - yes
    - A system-wide CPU time depletion could starve both the Health Monitor's background thread and
      the Launch Manager daemon concurrently, i.e. the detecting element (Launch Manager) could be
      starved at the same time as the monitored element, undermining the supervision mechanism's
      ability to detect and react to the very condition it is meant to catch. Extends the
      single-element mitigation already identified for the Health Monitor (``HM_FMEA_001`` in
      :need:`doc__health_monitor_fmea`) to the dependent-failure case; needs a corresponding
      assumption/requirement for the Launch Manager's own scheduling.
  * - UI_01_11
    - Memory depletion
    - yes
    - Raised conservatively as a potential common-cause resource exhaustion affecting both elements on
      the same target; no concrete historical incident is known for this specific failure mode at the
      time of writing. Needs confirmation/refinement by the Safety Manager or system architect
      (e.g. whether memory budgets are enforced per-process).
  * - UI_01_12
    - Other HW unavailability
    - yes
    - The Launch Manager's own liveliness is, in turn, monitored externally via a watchdog interface
      (see :need:`feat_arc_dyn__lifecycle__watch`); unavailability of that external watchdog hardware
      path is a dependent-failure concern shared between this feature and the external monitor.


DFA
===

For all identified applicable failure initiators, the DFA is performed in the following section.

.. code-block:: rst

    .. feat_saf_dfa:: <Title>
       :violates: <Feature architecture>
       :id: feat_saf_dfa__<Feature>__<Element descriptor>
       :failure_id: <ID from DFA failure initiators :need:`gd_guidl__dfa_failure_initiators`>
       :failure_effect: "description of failure effect of the failure initiator on the element"
       :mitigated_by: <ID from Feature Requirement | ID from AoU Feature Requirement>
       :mitigation_issue: <ID from Issue Tracker>
       :sufficient: <yes|no>
       :status: <valid|invalid>
       :version: 1

 .. note::   Argument is inside the 'content'. Therefore content is mandatory.

.. attention::
    The above directive must be updated according to your feature DFA.

    - The above "code-block" directive must be updated
    - Fill in all the needed information in the <brackets>
