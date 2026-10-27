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

Lifecycle Architecture Inspection
=================================

.. document:: Lifecycle Architecture Inspection
   :id: doc__lifecycle_arc_inspection
   :status: draft
   :version: 3
   :safety: ASIL_B
   :security: YES
   :realizes: wp__sw_arch_verification[version==1]

Participants
------------

.. note::

   As described in the concept :need:`doc_concept__wp_inspections` the following “inspection roles” are expected to be filled:

   * content responsible (author): <contributor/committer explicitly named here, who is the main author, as can be seen in config mgt tooling>
   * reviewer: <contributor/committer explicitly named here, who is the main content reviewer, must be different from content responsible>
   * moderator: <committer explicitly named here, who is is the safety manager, security manager or quality manager initiating the inspection>


.. list-table:: Architecture Inspection Participants
    :header-rows: 1

    * - Author(s)
      - Reviewer(s)
      - Moderator
    * - TODO: `<https://github.com/NN>`_
      - TODO: `<https://github.com/NN>`_
      - TODO: `<https://github.com/NN>`_


Architecture Inspection Checklist
---------------------------------

.. note::

   **Purpose**

   The purpose of the software architecture checklist is to ensure that the design meets the criteria and quality standards
   defined by project processes and guidelines for feature and component architectural design elements.
   It helps check compliance with requirements, identify errors or inconsistencies, and ensure adherence to best practices.
   The checklist guides the evaluation of the architecture design, identifies potential problems, and aids in communication and
   documentation of architectural decisions to stakeholders.

   **Checklist**

   Enter “yes” or “no” in the “Passed” column for each applicable checklist item, and explain the result in the “Remarks” column.
   If “no” is entered, add an issue link in the “Issue link” column unless the finding is already tracked in the current issue.
   If a Review ID is not applicable to your architecture, enter “n/a” in the “Passed” column and explain why in the “Remarks” column.
   See also :need:`doc_concept__wp_inspections` for further information about reviews in general and inspection in particular.

.. list-table:: Architecture Inspection Checklist
    :header-rows: 1

    * - Review ID
      - Acceptance criteria
      - Guidance
      - Passed
      - Remarks
      - Issue link
    * - ARC_01_01
      - Does the software architecture design take into account all requirements allocated to the architectural element, including functional, non-functional, safety, and security requirements, as well as all related design decisions?
      - Check whether all requirements allocated to the architectural element are considered in the design. These include functional requirements (e.g., functional safety requirements), non-functional requirements (e.g., performance and reliability), and security requirements (e.g., confidentiality and integrity). Also ensure that all related design decisions are taken into account and documented in the architectural design. Security-related requirements should also be reviewed through the applicable cybersecurity process; this checklist does not replace cybersecurity activities.
      - NO
      - All functional feature requirements are linked to feature architecture elements. Exception is feat_req__lifecycle__prog_lang.
        See the traceability overview here https://eclipse-score.github.io/lifecycle/main/verification_report/module_verification_report.html#traceability-of-valid-feature-requirements-to-feature-architecture
      - `#871 <https://github.com/eclipse-score/lifecycle/issues/871>`_
    * - ARC_01_02
      - If the architectural element is related to any supplier manuals (including safety and security), are the relevant parts covered?
      - If the architecture makes use of supplied elements, their manuals (e.g., safety manuals) must be considered; their functionality must match expectations, and their assumptions must be fulfilled. For a safety component, this means that the assumed Technical Safety Requirements and assumptions of use (AoUs) in the safety manual are covered.
      - NO
      - QNX safety manual is relevant for lifecycle, but currently out of scope.
        AoUs from dependent modules are currently not yet considered.
      - `#873 <https://github.com/eclipse-score/lifecycle/issues/873>`_, `#874 <https://github.com/eclipse-score/lifecycle/issues/874>`_
    * - ARC_01_03
      - Is the architectural element traceable to lower-level artifacts as defined by work-product traceability?
      - Check if every logical interface is provided by exactly one component and used by at least one. CHeck if every component of the feature is included by exactly one module.
      - NO
      - All existing logical interfaces are provided by a component, see https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/index.html#feat_arc_sta__lifecycle__static_view_arch
        Gap: Heartbeat Monitor interface of the Health Monitor component has no logical interface element.
      - `#872 <https://github.com/eclipse-score/lifecycle/issues/872>`_
    * - ARC_02_01
      - Is the software architecture design compliant with the overall feature architecture?
      - At the component level, check against the feature architecture. At the feature level, check other features that use common components.
      - YES
      - Dependencies to other modules checked for completeness, see https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/index.html#feat_arc_sta__lifecycle__static_view_arch
      -
    * - ARC_02_02
      - Are operations and interfaces named appropriately and comprehensibly in the architectural design?
      - Check :need:`gd_guidl__arch_design`
      - YES
      - Interface names and operations are clear. Launch Manager abstract interface elements also have additional description defining the input and output of operations.
        Improvement: Add additional description to Health Monitor interfaces as well.
      - `#875 <https://github.com/eclipse-score/lifecycle/issues/875>`_
    * - ARC_02_03
      - Has the correctness of data and control flows within the architectural elements been considered?
      - For example, examine data definitions, transformations, integrity, and interactions; check error handling, data exchange between elements, correct responses to inputs, and documented decision-making.
        Note: Consistency is ensured by the process and tooling, which define each interface only once.
      - YES
      - See for example sequence diagrams `feat_arc_dyn__lifecycle__ctrl_que_act <https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/run_target_activation.html#feat_arc_dyn__lifecycle__ctrl_que_act>`_, `feat_arc_dyn__lifecycle__alive_monitor_start <https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/health_monitoring.html#feat_arc_dyn__lifecycle__alive_monitor_start>`_, `feat_arc_dyn__lifecycle__comp_sm_if <https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/components_run_targets.html#feat_arc_dyn__lifecycle__comp_sm_if>`_ where error conditions are highlighted.
      -
    * - ARC_02_04
      - Are the interfaces between the software architectural element and other architectural elements well defined and described?
      - Check whether the interface description specifies expected behaviour for invalid inputs and errors. Could established protocols be used? Does the interface description document inputs, outputs, and error handling? Have loose coupling and limited exposure been considered? Can unit or integration tests be written against the interface? Has the amount of data transferred been considered appropriately? Is sensitive data protected appropriately, with no sensitive data exposed?
      - NO
      - Interface operation definitions for Launch Manager interfaces have extended descriptions of possible inputs and outputs. Health Monitor interfaces currently lack such descriptions.
      - `#875 <https://github.com/eclipse-score/lifecycle/issues/875>`_
    * - ARC_02_05
      - Does the software architectural element take applicable timing requirements into account?
      - If there are strict timing requirements, execution-time analysis or estimation should be performed, and appropriate deadline monitoring should be considered.
      - n/a
      - No timing requirements are defined for the lifecycle feature.
      -
    * - ARC_02_06
      - Is the documentation of the software architectural element, including textual and graphical descriptions (e.g., UML diagrams), clear and complete?
      - The use of semi-formal notation is expected for architectural elements with an allocated ASIL level. Is the architecture template filled out correctly?
      - YES
      - See UML diagrams in https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/run_target_activation.html
      -
    * - ARC_03_01
      - Is the architectural element modular and encapsulated?
      - Check, for example, that only the necessary interfaces are used and that interfaces and interactions are clearly defined. Project-specific design and coding guidelines may additionally require object-oriented design, appropriate use of access controls (e.g., private or protected), and limits on global variables.
      - YES
      - Interfaces are object oriented and kept minimal.
      -
    * - ARC_03_02
      - Is the suitability of the software architecture for future modifications and maintainability considered?
      - Check for loose coupling, separation of concerns, high cohesion, an interface versioning strategy, decision records, and the use of established design patterns.
      - YES
      - The two components Launch Manager and Health Monitor are loosely coupled with only a minimal interface. Each component could be used without the other if desired.
      -
    * - ARC_03_03
      - Does the software architecture demonstrate simplicity and avoid unnecessary complexity?
      - Indicators of complexity include the number of use cases (corresponding to dynamic diagrams) allocated to a single design element; the number of interfaces and operations in an interface; the number of function parameters; global variables; complex types; and limited comprehensibility. The thresholds below are project-defined criteria, not ISO 26262 limits.

        Notes:

        A design rationale is mandatory if any of the following applies: the number of use cases or interfaces exceeds "3"; the number of function parameters exceeds "5"; the number of operations exceeds "20"; or global variables are used.
      - YES
      - The architecture is kept simple with minimal interfaces and clear separation of concerns. Number of function parameters and operations is not exceeded.
        The number of interfaces is up to 4, due to keeping interfaces minimal and having dedicated interfaces for each functionality. See https://eclipse-score.github.io/score/main/features/lifecycle/architecture/index.html
      -
    * - ARC_03_04
      - Does the software architecture design follow best practices and design principles?
      - Refer to architectural guidelines and recommendations within the project documentation.
      - YES
      - Guideline :need:`gd_guidl__arch_design` is followed.
      -
    * - ARC_04_01
      - Where software elements with different safety classifications (e.g., different ASILs, or QM and ASIL) share resources, have applicable freedom-from-interference requirements been identified and addressed? Consider shared resources such as CPU time and memory. See also ARC_04_03.

        Note: See :need:`std_req__iso26262__software_749` and Annex D for partitioning and freedom-from-interference considerations.
      - Check whether your architecture design complies with project guidelines for establishing freedom from interference between components. This can be achieved, for example, by using a hypervisor or an OS that supports partitioning with an MMU or specific scheduling mechanisms, as well as safety mechanisms such as watchdogs or program-flow monitoring.
        Identify the shared resources and potential interference paths, and check that the selected measures address the applicable requirements. A hypervisor, operating system feature, or safety mechanism is not by itself evidence that freedom from interference has been achieved. Where the design relies on operating-system or platform guarantees, verify that these guarantees are specified and supported by applicable assumptions of use. For example, see `score aou_req__platform__process_isolation <https://eclipse-score.github.io/score/main/requirements/platform_assumptions/index.html#aou_req__platform__process_isolation>`_.
      - YES
      - Both components Launch Manager and Health Monitor are rated ASIL-B.
      -
    * - ARC_04_02
      - Does the software architectural design consider feasibility with respect to the resources required by the embedded software, especially time-critical aspects such as startup time, as well as RAM, ROM, non-volatile memory, communication bandwidth, and processing-time limits, according to requirements or foreseeable customer needs? See also ARC_02_05.

        Note: See :need:`std_req__iso26262__software_7413`.
      -
        Check whether there are any limits on resource consumption or timing in your project, such as startup time, communication bandwidth, or memory usage. If such limits exist, ensure that your architecture takes them into account, especially with respect to scalability. To do this, estimate the required resources based on the architectural design and a prototype or measurements of an existing implementation, and compare the estimates with the defined limits or planned scalability. Check whether any bottlenecks in the architecture could lead to excessive resource use or timing violations.
      - YES
      - There are no concrete resource requirements, though in general concerns like startup time have been considered in the design. See design decisions `launch-manager-starts-processes-on-a-thread-pool <https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/design_decisions.html#launch-manager-starts-processes-on-a-thread-pool>`_, `launch-manager-alive-monitoring-is-polling-based <https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/design_decisions.html#launch-manager-alive-monitoring-is-polling-based>`_, `dec_rec__lifecycle__flatbuffer_config <https://eclipse-score.github.io/lifecycle/main/features/lifecycle/architecture/design_decisions.html#dec_rec__lifecycle__flatbuffer_config>`_
      -
    * - ARC_04_03
      - If your software architectural design includes processes and tasks, are their scheduling policies and priorities (or at least the necessary relationships among them) defined to ensure that timing requirements are met? Please note that the specific priorities or priority ranges will probably be defined in the project handbook or the software development plan.

        Note: See :need:`std_req__iso26262__software_743`.
      - Provide a rationale for these scheduling policies and priorities, or explain why they are not needed.
      - NO
      - Scheduling concept currently missing on platform level.
      - `#880 <https://github.com/eclipse-score/lifecycle/issues/880>`_

Summary
-------

Inspected Static Architecture Views
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following static views in "valid_inspected" state are in the scope of this inspection:

.. needtable::
   :filter: "lifecycle" in docname and "architecture" in docname and docname is not None
   :style: table
   :types: feat_arc_sta
   :tags: lifecycle
   :columns: id;status;tags
   :colwidths: 25,25,25
   :sort: title

Inspected Dynamic Architecture Views
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

and the following dynamic views in "valid_inspected" state are in the scope of this inspection:

.. needtable::
   :filter: "lifecycle" in docname and "architecture" in docname and docname is not None
   :style: table
   :types: feat_arc_dyn
   :tags: lifecycle
   :columns: id;status;tags
   :colwidths: 25,25,25
   :sort: title

.. note::

   For these static and dynamic views the inspected ground truth is documented in the rendered documentation and not in the source read by the drawing tool.
