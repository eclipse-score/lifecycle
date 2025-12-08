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

.. _component_architecture_hm:

Component Architecture
======================

.. document:: HealthMonitor Architecture
   :id: doc__hm_architecture
   :status: draft
   :safety: ASIL_B
   :security: NO
   :realizes: wp__component_arch
   :tags: template

.. attention::
    The above directive must be updated according to your needs.

    - Modify ``Your Component Name`` to be your Component Name
    - Modify ``id`` to be your Component Name in upper snake case preceded by ``doc__`` and followed by ``_architecture``
    - Adjust ``status`` to be ``valid``
    - Adjust ``safety`` and ``tags`` according to your needs

Overview
--------

Document describes HealthMonitor component architecture.

Description
-----------

This component provides a set of functionalities to manage the lifecycle of programs running on the S-CORE platform.

Requirements Linked to Component Architecture
---------------------------------------------

TODO

Rationale Behind Architecture Decomposition
*******************************************

Architecture is not decomposed.

Static Architecture
-------------------

.. comp_arc_sta:: Deadline Monitor
   :id: comp_arc_sta__dm__static_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :implements:
   :fulfils:
   :includes:

   .. uml::  assets/dm_static_architecture.puml

.. comp_arc_sta:: Logic Monitor
   :id: comp_arc_sta__lm__static_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :implements:
   :fulfils:
   :includes:

   .. uml::  assets/lm_static_architecture.puml

.. comp_arc_sta:: Health Monitor  
   :id: comp_arc_sta__hm__static_view
   :security: NO
   :safety: ASIL_B
   :status: valid
   :implements:
   :fulfils:
   :includes:

   .. uml::  assets/hm_static_architecture.puml

Dynamic Architecture
--------------------

.. comp_arc_dyn:: HealthMonitor Creation
   :id: comp_arc_dyn__hm__creation
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils:

   .. uml::  assets/hm_creation.puml

.. comp_arc_dyn:: DeadlineMonitor Usage
   :id: comp_arc_dyn__dm__usage
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils:

   .. uml::  assets/dm_usage.puml

.. comp_arc_dyn:: LogicMonitor Usage
   :id: comp_arc_dyn__lm__usage
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils:

   .. uml::  assets/lm_usage.puml

Interfaces
----------

.. real_arc_int:: DeadlineMonitor Interface
   :id: real_arc_int__dm__interface
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils:
   :language: rust

   .. uml::  assets/dm_interface.puml

.. real_arc_int:: LogicMonitor Interface
   :id: real_arc_int__lm__interface
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils:
   :language: rust

   .. uml::  assets/lm_interface.puml

.. real_arc_int:: HealthMonitor Interface
   :id: real_arc_int__hm__interface
   :security: NO
   :safety: ASIL_B
   :status: valid
   :fulfils:
   :language: rust

   .. uml::  assets/hm_interface.puml

Lower Level Components
----------------------

.. comp_arc_sta:: Component Name 2
   :id: comp_arc_sta__hm__2
   :status: invalid
   :safety: ASIL_B
   :security: YES
   :fulfils: comp_req__hm__some_title
   :implements: logic_arc_int__feature_name__interface_name

   no architecture but detailed design

.. note::
   Architecture can be split into multiple files. At component level the public interfaces to be used by the user and tester to be shown.

.. attention::
    The above directives must be updated according to your component architecture.

    - Replace the example content by the real content (according to :need:`gd_guidl__arch_design`)
    - Set the status to valid and start the review/merge process
