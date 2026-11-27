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

.. _component_detailed_design_hm:

Detailed Design
###############

.. document:: HealthMonitor Detailed Design
   :id: doc__health_monitor_detailed_design
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__sw_implementation[version==1]
   :tags: template

.. attention::
    The above directive must be updated according to your Component.

    - Modify ``Your Component Name`` to be your Component Name
    - Modify ``id`` to be your Component Name in upper snake case preceded by ``doc__`` and followed by ``_detailed_design``
    - Adjust ``status`` to be ``valid``
    - Adjust ``safety`` and ``tags`` according to your needs

Detailed Design for Component: HealthMonitor
===============================================

Description
-----------

HealthMonitor component consists of these units:

- `deadline_monitor`
- `heartbeat_monitor`
- `logic_monitor`
- `health_monitor`

| Design Decisions - For the documentation of the decision the :need:`gd_temp__change_decision_record` can be used.
| Design Constraints


Implementation structure
------------------------

.. code-block:: bash

    src
    ├── cpp                     # C++ implementation, public headers included as score/health_monitor/<file>.h
    │   ├── deadline_monitor
    │   ├── heartbeat_monitor
    │   ├── logic_monitor
    │   ├── internal            # Worker thread, monitoring logic and thread setup
    │   └── supervisor_client   # Launch manager alive client and stub backend
    └── rust                    # Rust implementation and its C-compatible FFI
        ├── deadline
        ├── heartbeat
        ├── logic
        └── supervisor_api_client

Implementation approach for multi language
*******************************************

The Health Monitor has two independent implementations with the same monitoring behaviour: `Rust` in ``src/rust`` and `C++` in ``src/cpp``.
An application uses the implementation of its own language and links no code of the other.
The `Rust` implementation also provides a C-compatible Foreign Function Interface (FFI); no `C++` code uses it.

Rationale Behind Decomposition into Units
******************************************

| mandatory: a motivation for the decomposition into one or more units.

.. note:: Reason for split into multiple units could be-
	    - Based on design principles like SOLID, DRY, etc.
	    - Based on design patterns, etc.

Static Diagrams for Unit Interactions
-------------------------------------

.. uml::  assets/static_diagram.puml

Dynamic Diagrams for Unit Interactions
--------------------------------------
.. code-block:: rst

   .. dd_dyn:: <Title>
      :id: dd_dyn__<Feature>__<Title>
      :security: <YES|NO>
      :safety: <QM|ASIL_B>
      :status: <valid|invalid>
      :version: 1
      :implements: <link to component requirement id>
      :satisfies: <link to component architecture id>

        .. image:: <link to drawio image> or .. uml::  assets/<link to plantuml>
