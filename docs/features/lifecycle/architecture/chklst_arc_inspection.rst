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


.. document:: Lifecycle Architecture Inspection Checklist
   :id: doc__lifecycle_arc_inspection
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: YES
   :realizes: wp__sw_arch_verification[version==1]

Architecture Inspection Checklist
=================================

Purpose
-------

The purpose of the software architecture checklist is to ensure that the design meets the criteria and quality as
defined per project processes and guidelines for feature and component architectural design elements.
It helps to check the compliance with requirements, identify errors or inconsistencies, and ensure adherence to best
practices.
The checklist guides evaluation of the architecture design, identifies potential problems, and aids in
communication and documentation of architectural decisions to stakeholders.

Conduct
-------

As described in the concept :need:`doc_concept__wp_inspections` the following "inspection roles" are expected to be filled:

- content responsible (author): <contributor/committer explicitly named here, who is the main author, as can be seen in config mgt tooling>
- reviewer: <contributor/committer explicitly named here, who is the main content reviewer, must be different from content responsible>
- moderator: <committer explicitly named here, who is is the safety manager, security manager or quality manager initiating the inspection>

Checklist
---------

It is mandatory to fill in the "passed" column with "yes" or "no" for each checklist item and additionally to add in the remarks why it is passed or not passed.
In case of "no" an issue link to the issue tracking system has to be added in the last column (if not solved in the same issue).
See also :need:`doc_concept__wp_inspections` for further information about reviews in general and inspection in particular.

.. list-table:: Architecture Design Review Checklist
    :header-rows: 1

    * - Review Id
      - Acceptance criteria
      - Guidance
      - passed
      - Remarks
      - Issue link
    * - ARC_01_01
      - Is the traceability from software architectural elements to requirements, and other level architectural elements (e.g. component to interface) established according to the "Relations between the architectural elements" as described in :need:`doc_concept__arch_process`?
      - Trace should be checked automatically by tool support in the future. It will be removed from the checklist once the requirement (:need:`Correlations of the architectural building blocks <gd_req__arch_build_blocks_corr>`) is implemented. Refer to `Tool Requirements <https://eclipse-score.github.io/docs-as-code/main/internals/requirements/requirements.html>`_ for the current status.
      - NO
      - All functional feature requirements are linked to feature architecture elements. Exception is feat_req__lifecycle__prog_lang
        See the traceability overview here https://eclipse-score.github.io/lifecycle/main/verification_report/module_verification_report.html#traceability-of-valid-feature-requirements-to-feature-architecture
        All logical interfaces are linked to components.
        Gap: Hearbeat Monitor interface for HealthMonitor component has no logical interface element.
      -
    * - ARC_01_02
      - Does the software architecture design consider all the requirements allocated to the architectural element, including functional, non-functional, safety, and security requirements and all related design decisions?
      - Check if all requirements allocated to the architectural element are considered in the design. This includes functional requirements (e.g. functional safety requirements), non-functional requirements (e.g. performance, reliability), and security requirements (e.g. confidentiality, integrity). Additionally, ensure that all related design decisions are taken into account and documented in the architectural design.
      - NO
      - All functional feature requirements are linked to feature architecture elements. Exception is feat_req__lifecycle__prog_lang
        See the traceability overview here https://eclipse-score.github.io/lifecycle/main/verification_report/module_verification_report.html#traceability-of-valid-feature-requirements-to-feature-architecture
      -
    * - ARC_01_03
      - If the architectural element is related to any supplier manuals (including safety and security), are the relevant parts covered?
      - If the architecture makes use of supplied elements, their manuals (like safety) have to be considered (i.e. its provided functionality matches the expectation and assumptions are fulfilled). Note that in case of safety component this means that assumed Technical Safety Requirements and AoUs of the safety manual are covered.
      - NO
      - QNX safety manual is relevant for lifecycle, but currently out of scope.
        AoUs from dependent modules are currently not yet considered.
      -
    * - ARC_01_04
      - Is the architectural element traceable to the lower-level artifacts as defined by the work product traceability?
      - 
      - YES
      - All existing logical interface elements are traced to components, see https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/index.html#feat_arc_sta__lifecycle__static_view_arch
      -
    * - ARC_02_01
      - Is the software architecture design compliant with the overall feature architecture?
      - On component level check against the feature architecture, on feature level check other features with common components used.
      - YES
      - Dependencies to other modules checked for completeness, see https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/index.html#feat_arc_sta__lifecycle__static_view_arch
      -
    * - ARC_02_02
      - Is appropriate and comprehensible operation and interface naming present in the architectural design?
      - Check :need:`gd_guidl__arch_design`
      - YES
      - Interface names and Operations are clear. Launch Manager abstract interface elements also have additional description defining the input and output of operations.
        Improvement: Add additional description to Health Monitor interfaces as well.
      -
    * - ARC_02_03
      - Are the correctness of data flow and control flow within the architectural elements considered?
      - For example, examine definitions, transformations, integrity, and interaction of data; check error handling, data exchange between elements, correct response to inputs, and documented decision making.
        Note: Consistency is ensured by the process/tooling, by defining each interface only once.
      - YES
      - See for example sequence diagrams `feat_arc_dyn__lifecycle__lcm_term <https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/run_target_activation.html#feat_arc_dyn__lifecycle__lcm_term>`_, `feat_arc_dyn__lifecycle__alive_monitor_start <https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/health_monitoring.html#feat_arc_dyn__lifecycle__alive_monitor_start>`_, `feat_arc_dyn__lifecycle__comp_sm_if <https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/components_run_targets.html#feat_arc_dyn__lifecycle__comp_sm_if>`_ where error conditions are highlighted.
      -
    * - ARC_02_04
      - Are the interfaces between the software architectural element and other architectural elements well defined?
      - Check if the interface handles undefined behaviour or errors; can established protocols be used; are the interfaces for inputs, outputs, and error codes documented; is loose coupling considered and only limited exposure; can unit or integration tests be written against the interface; data amount transferred; ensure no sensitive data is exposed;
      - NO
      - Interface Operations definitions for Launch Manager interfaces has extended description about possible input and outputs. Health Monitor interfaces currently lack such descriptions.
      -
    * - ARC_02_05
      - Does the software architectural element consider the timing constraints (from the parent requirement)?
      - If there are strict timing requirements, a programming time estimation should be performed and deadline supervision should be considered.
      - YES
      - Not applicable, no timing constraints defined.
      -
    * - ARC_02_06
      - Is the documentation of the software architectural element, including textual and graphical descriptions (e.g., UML diagrams), clear and complete?
      - Use of semi-formal notation is expected for architectural elements with an allocated ASIL level. Is the architecture template correctly filled?
      - <Add diagram for control interface>, Check arch template
      -
      -
    * - ARC_03_01
      - Is the architectural element modular and encapsulated?
      - Check, for example, that only minimal interfaces are used. The design should be object oriented. Interfaces and interactions are clearly defined. Usage of access types (private, protected) is properly set. Limited global variables.
      - YES
      - Interfaces are object oriented and kept minimal.
      -
    * - ARC_03_02
      - Is the suitability of the software architecture for future modifications and maintainability considered?
      - Check for, for example, loose coupling, separation of concerns, high cohesion, versioning strategy for interfaces, decision records, and use of established design patterns.
      - YES
      - The two components Launch Manager and Health Monitor are loosely coupled with only a minimal interface. Each component could be used without the other if desired.
      -
    * - ARC_03_03
      - Are simplicity and avoidance of unnecessary complexity present in the software architecture?
      - Indicators of complexity include: the number of use cases (corresponding to dynamic diagrams) allocated to a single design element, the number of interfaces and operations in an interface, function parameters, global variables, complex types, and limited comprehensibility.

        Notes:

        If the "number of use cases" or "number of interfaces" above exceeds "3" or "number of function parameters" exceeds "5" or the "number of operations" exceeds "20" or global variables are used, a design rationale is mandatory.
      - YES
      - The architecture is kept simple with minimal interfaces and clear separation of concerns. Number of function parameters and operations is not exceeded.
        The number of interfaces is up to 4, due to keeping interfaces minimal and having dedicated interfaces for each functionality. See https://eclipse-score.github.io/score/main/features/lifecycle/architecture/index.html
      -
    * - ARC_03_04
      - Is the software architecture design following best practices and design principles?
      - Refer to architectural guidelines and recommendations within the project documentation.
      - YES
      - Guideline `gd_guidl__arch_design <https://eclipse-score.github.io/process_description//main/process_areas/architecture_design/guidance/architecture_guideline.html#gd_guidl__arch_design>`_ is followed,
      -
    * - ARC_04_01
      - If your software architectural design includes processes with different safety ratings (QM/ASIL), is freedom from interference for shared resources (CPU time, shared memory, etc.) ensured? See also ARC_04_03.

        Note: see :need:`std_req__iso26262__software_7411` and :need:`std_req__iso26262__software_749` with Annex D for partitioning to ensure freedom from interference.
        Note: Modules should not mix ASIL and QM processes unless justified otherwise; therefore, this question is only relevant on the feature level.
      -
        Check whether your architecture design complies with project guidelines to establish freedom from interference between components. This can be achieved, for example, by using a hypervisor or an OS that supports partitioning with an MMU or specific scheduling mechanisms, as well as safety mechanisms like watchdogs or program flow monitoring.
        Also check if the operating system supports freedom from interference between the processes and make sure an "Assumption of Use requirement" for this exists in your project. For example, see `score aou_req__platform__process_isolation <https://eclipse-score.github.io/score/main/requirements/platform_assumptions/index.html#aou_req__platform__process_isolation>`_.
      - YES
      - Both components Launch Manager and Health Monitor are rated ASIL-B.
      -
    * - ARC_04_02
      - Does the software architectural design consider its feasibility with respect to the required resources for the embedded software, especially for time-critical aspects like startup time, but also including RAM, ROM, non-volatile memory, communication bandwidth, and processing time limits according to the requirements or foreseeable customer needs? See also ARC_02_05.

        Note: see :need:`std_req__iso26262__software_7413`
      -
        Check if there are any limits for resource consumption or timing aspects in your project, such as startup time, communication bandwidth, or memory usage. If such limits exist, ensure that your architecture takes these limits into account, especially with respect to scalability. For this, make an estimation of the required resources based on the architectural design and a prototypical implementation or a measurement of an existing implementation, and compare it to the defined limits or planned scalability. Check if any bottlenecks are present in the architecture that could lead to resource overuse or timing violations.
      - YES
      - There are no concrete resource requirements, though in general concerns like startup time have been considered in the desin. See design decisions `launch-manager-starts-processes-on-a-thread-pool <https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/design_decisions.html#launch-manager-starts-processes-on-a-thread-pool>`_, `launch-manager-alive-monitoring-is-polling-based <https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/design_decisions.html#launch-manager-alive-monitoring-is-polling-based>`_, `dec_rec__lifecycle__flatbuffer_config <https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/design_decisions.html#dec_rec__lifecycle__flatbuffer_config>`_
      -
    * - ARC_04_03
      - If your software architectural design includes processes and tasks, are their scheduling policies and priorities (at least the necessary relationships between them) defined to ensure that timing requirements are met? Please note that the particular priorities or priority ranges will probably be defined by the project handbook or the software development plan.

        Note: see :need:`std_req__iso26262__software_743`
      - Provide a rationale for these scheduling policies and priorities, or explain why they are not needed.
      - NO
      - Scheduling concept currently missing on platform level.
      -


.. attention::
    The above checklist entries must be filled according to your feature architecture in scope.

Note: If a Review ID is not applicable for your architecture, then state ""n/a" in status and comment accordingly in remarks.

The following static views in "valid_inspected" state are in the scope of this inspection:

.. needtable::
   :filter: "lifecycle" in docname and "architecture" in docname and docname is not None
   :style: table
   :types: feat_arc_sta
   :tags: lifecycle
   :columns: id;status;tags
   :colwidths: 25,25,25
   :sort: title

and the following dynamic views:

.. needtable::
   :filter: "lifecycle" in docname and "architecture" in docname and docname is not None
   :style: table
   :types: feat_arc_dyn
   :tags: lifecycle
   :columns: id;status;tags
   :colwidths: 25,25,25
   :sort: title
