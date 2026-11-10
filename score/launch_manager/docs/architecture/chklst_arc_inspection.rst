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

Launch Manager Architecture Inspection
======================================

.. document:: Launch Manager Architecture Inspection
  :id: doc__launch_manager_arc_inspection
  :status: draft
  :version: 1
  :safety: ASIL_B
  :security: YES
  :realizes: wp__sw_arch_verification[version==1]

Participants
------------

.. note::

   As described in the concept :need:`doc_concept__wp_inspections` the following “inspection roles” are expected to be filled:

   * content responsible (author): <contributor/committer explicitly named here, who is the main author, as can be seen in config mgt tooling>
   * reviewer: <contributor/committer explicitly named here, who is the main content reviewer, must be different from content responsible>
   * moderator: <committer explicitly named here, who is the safety manager, security manager or quality manager initiating the inspection>


.. list-table:: Architecture Inspection Participants
    :header-rows: 1

    * - Author(s)
      - Reviewer(s)
      - Moderator
    * - `<https://github.com/NN>`_, `<https://github.com/NN>`_
      - `<https://github.com/NN>`_, `<https://github.com/NN>`_
      - `<https://github.com/NN>`_

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
      -
      -
      -
    * - ARC_01_02
      - If the architectural element is related to any supplier manuals (incl. safety and security)
        are the relevant parts covered?
      - If the architecture makes use of supplied elements, their manuals (e.g., safety manuals) must be considered; their functionality must match expectations, and their assumptions must be fulfilled. For a safety component, this means that the assumed Technical Safety Requirements and assumptions of use (AoUs) in the safety manual are covered.
      -
      -
      -
    * - ARC_01_03
      - Is the architectural element traceable to lower level artifacts as defined by workproduct traceability?
      - All components defined in the architecture should be traceable to source (folders) and their (executable) build instructions.
      -
      -
      -
    * - ARC_02_01
      - Is the software architecture design compliant with the overall feature architecture?
      - At the component level, check against the feature architecture. At the feature level, check other features that use common components.
      -
      -
      -
    * - ARC_02_02
      - Are operations and interfaces named appropriately and comprehensibly in the architectural design?
      - Check :need:`gd_guidl__arch_design`
      -
      -
      -
    * - ARC_02_03
      - Has the correctness of data and control flows within the architectural elements been considered?
      - For example, examine data definitions, transformations, integrity, and interactions; check error handling, data exchange between elements, correct responses to inputs, and documented decision-making.
        Note: Consistency is ensured by the process and tooling, which define each interface only once.
      -
      -
      -
    * - ARC_02_04
      - Are the interfaces between the software architectural element and other architectural elements well defined and described?
      - Check whether the interface description specifies expected behaviour for invalid inputs and errors. Could established protocols be used? Does the interface description document inputs, outputs, and error handling? Have loose coupling and limited exposure been considered? Can unit or integration tests be written against the interface? Has the amount of data transferred been considered appropriately? Is sensitive data protected appropriately, with no sensitive data exposed?
      -
      -
      -
    * - ARC_02_05
      - Does the software architectural element take applicable timing requirements into account?
      - If there are strict timing requirements, execution-time analysis or estimation should be performed, and appropriate deadline monitoring should be considered.
      -
      -
      -
    * - ARC_02_06
      - Is the documentation of the software architectural element, including textual and graphical descriptions (e.g., UML diagrams), clear and complete?
      - The use of semi-formal notation is expected for architectural elements with an allocated ASIL level. Is the architecture template filled out correctly?
      -
      -
      -
    * - ARC_03_01
      - Is the architectural element modular and encapsulated?
      - Check, for example, that only the necessary interfaces are used and that interfaces and interactions are clearly defined. Project-specific design and coding guidelines may additionally require object-oriented design, appropriate use of access controls (e.g., private or protected), and limits on global variables.
      -
      -
      -
    * - ARC_03_02
      - Is the suitability of the software architecture for future modifications and maintainability considered?
      - Check for loose coupling, separation of concerns, high cohesion, an interface versioning strategy, decision records, and the use of established design patterns.
      -
      -
      -
    * - ARC_03_03
      - Does the software architecture demonstrate simplicity and avoid unnecessary complexity?
      - Indicators of complexity include the number of use cases (corresponding to dynamic diagrams) allocated to a single design element; the number of interfaces and operations in an interface; the number of function parameters; global variables; complex types; and limited comprehensibility. The thresholds below are project-defined criteria, not ISO 26262 limits.

        Notes:

        A design rationale is mandatory if any of the following applies: the number of use cases or interfaces exceeds "3"; the number of function parameters exceeds "5"; the number of operations exceeds "20"; or global variables are used.
      -
      -
      -
    * - ARC_03_04
      - Does the software architecture design follow best practices and design principles?
      - Refer to architectural guidelines and recommendations within the project documentation.
      -
      -
      -
    * - ARC_04_03
      - If your software architectural design of the component includes processes and tasks, are their scheduling policies and priorities (or at least the necessary relationships among them) defined to ensure that timing requirements are met? Please note that the specific priorities or priority ranges will probably be defined in the project handbook or the software development plan.

        Note: See :need:`std_req__iso26262__software_743`.
      - Give a reason for these scheduling policies and priorities or explain why not needed.
      -
      -
      -


Summary
-------

Inspected Static Architecture Views
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following static views in "valid_inspected" state are in the scope of this inspection:

.. needtable::
   :filter: "launch_manager" in docname and "architecture" in docname and docname is not None
   :style: table
   :types: comp_arc_sta
   :tags: launch_manager
   :columns: id;status;tags
   :colwidths: 25,25,25
   :sort: title

Inspected Dynamic Architecture Views
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

and the following dynamic views in "valid_inspected" state are in the scope of this inspection:

.. needtable::
   :filter: "launch_manager" in docname and "architecture" in docname and docname is not None
   :style: table
   :types: comp_arc_dyn
   :tags: launch_manager
   :columns: id;status;tags
   :colwidths: 25,25,25
   :sort: title


.. note::

   For these static and dynamic views the inspected ground truth is documented in the rendered documentation and not in the source read by the drawing tool.
