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


DFA (Dependent Failure Analysis)
================================

.. document:: Health Monitor DFA
   :id: doc__health_monitor_dfa
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__sw_component_dfa[version==1]
   :tags: template

.. note:: Use the content of the document to describe e.g. why a fault model is not applicable for the diagram.

.. attention::
    **Draft status:** The applicability assessment below is a first analysis proposal prepared to
    support the module's Safety Manager, derived from the already accepted Health Monitor FMEA
    (:need:`doc__health_monitor_fmea`, see ``HM_FMEA_001``/``HM_FMEA_003``). It has **not yet been
    formally reviewed/accepted**. A structured ``comp_saf_dfa`` entry is intentionally not yet added,
    as a proper ``mitigated_by`` link requires a real, reviewed component requirement, which is not yet
    available for this component (see the deviations tracked in :need:`doc__lifecycle_safety_plan`).

Dependent Failure Initiators
----------------------------

Shared resources
^^^^^^^^^^^^^^^^

Per :need:`gd_guidl__dfa_failure_initiators`, this category is primarily intended for the Platform DFA.
However, the Health Monitor is itself a library linked into the address space of an arbitrary monitored
application (see :need:`doc__health_monitor_fmea`), so it is directly affected by several of these
initiators already at component level. Applicability is assessed below for component-level awareness;
a full cross-feature/cross-application assessment remains the responsibility of the Platform DFA.
Initiators about inter-element communication and shared configuration data are not repeated here, as
they are already assessed between the Health Monitor and the Launch Manager in the feature-level DFA
(:need:`doc__lifecycle_dfa`).

.. list-table:: DFA shared resources (component-level awareness)
  :header-rows: 1
  :widths: 10,20,10,20

  * - ID
    - Violation cause shared resources
    - Applicability
    - Rationale
  * - SR_01_01
    - Reused software components
    - yes
    - The Health Monitor library is linked into every monitored application across the platform; a
      single defect could therefore cause simultaneous failures in multiple, otherwise independent
      applications. Component-level awareness only; full cross-application impact assessment belongs
      to the Platform DFA.
  * - SR_01_02
    - Libraries
    - yes
    - This is the primary concern already identified as ``HM_FMEA_003`` in :need:`doc__health_monitor_fmea`:
      the Health Monitor library executes in the same process as the monitored application and any
      other co-resident code.
  * - SR_01_04
    - Basic software
    - no
    - No dependency on a shared "basic software" layer beyond the operating system (see SR_01_05) is
      documented for this component.
  * - SR_01_05
    - Operating system including scheduler
    - yes
    - Matches ``HM_FMEA_001`` in :need:`doc__health_monitor_fmea`: the Health Monitor's background
      supervision thread depends on being scheduled by the same OS scheduler as all other
      threads/processes on the target, including the monitored application itself.
  * - SR_01_06
    - Any service stack, e.g. communication stack
    - no
    - No communication-stack dependency is documented for the Health Monitor in isolation; its only
      external communication (the Alive interface towards the Launch Manager) is already assessed at
      feature level (:need:`doc__lifecycle_dfa`, category "Communication between the two elements").
  * - SR_01_07
    - Configuration data
    - no
    - Already assessed between the Health Monitor and the Launch Manager at feature level
      (:need:`doc__lifecycle_dfa`, ``SI_01_02``); not repeated here to avoid duplicate analysis.
  * - SR_01_09
    - Execution time
    - no
    - Same underlying concern as SR_01_05 (OS scheduler); not repeated here to avoid duplicate analysis.
  * - SR_01_10
    - Allocated memory
    - yes
    - Matches ``HM_FMEA_003``: the Health Monitor's internal monitoring data structures reside in the
      same allocated address space as the rest of the monitored application's memory, and can be
      corrupted by it (bitflips, out-of-range writes, etc.). The FMEA's accepted mitigation
      ("protected pages" around the monitoring data structures) is the currently implemented
      detection mechanism for this risk.

.. code-block:: rst

    .. comp_saf_dfa:: <Title>
       :violates: <Component architecture>
       :id: comp_saf_dfa__<Component>__<Element descriptor>
       :failure_id: <ID from DFA failure initiators :need:`gd_guidl__dfa_failure_initiators`>
       :failure_effect: "description of failure effect of the failure initiator on the element"
       :mitigated_by: <ID from Component Requirement | ID from AoU Component Requirement>
       :mitigation_issue: <ID from Issue Tracker>
       :sufficient: <yes|no>
       :status: <valid|invalid>
       :version: 1

.. note::   argument is inside the 'content'. Therefore content is mandatory

.. attention::
    The above directive must be updated according to your component DFA.

    - The above "code-block" directive must be updated
    - Fill in all the needed information in the <brackets>
