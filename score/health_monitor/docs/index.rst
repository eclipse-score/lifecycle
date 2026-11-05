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

.. _component_hm:

HealthMonitor
#####################

.. note:: Document header

.. document:: HealthMonitor
   :id: doc__health_monitor
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: YES
   :realizes: wp__cmpt_request[version==1]
   :tags: health_monitor

.. attention::
    The above directive must be updated according to your Component.

    - Modify ``document`` to be your Component Name
    - Modify ``id`` to be your Component Name in lower snake case preceded by ``doc__``
    - Adjust ``status`` to be ``valid``
    - Adjust ``safety``, ``security`` and ``tags`` according to your needs

.. note::
   This document is the Component Request (CR) of the component. It describes the current state of the component.
   Content that belongs to a change of the component, like rationale, rejected ideas, open issues and the impact on
   safety, security and license, is documented in a Decision Record (:need:`gd_temp__change_decision_record`) and,
   if needed, in an Impact Analysis (:need:`gd_temp__change_impact_analysis`).

Abstract
========

[A short (~200 word) description of the component.]

Motivation
==========

[Explain why the component is needed: which features or stakeholder needs it serves and why existing components are not sufficient.]

Specification
=============

[Describe the requirements and architecture of the component. Details are documented in the documents listed in Component Detail Information.]


How to Teach This
=================

[How to teach users, new and experienced, how to apply the CR to their work.]

.. note::
   For a CR that adds new functionality or changes behaviour, it is helpful to include a section on how to teach users, new and experienced, how to apply the CR to their work.

Footnotes
=========

[A collection of footnotes cited in the CR, and a place to list non-inline hyperlink targets.]


Component Detail Information
============================

Further documentation of the component can be found in the following documents:

.. toctree::
   :maxdepth: 1

   requirements/index
   architecture/index
   detailed_design/index
   safety_analysis/fmea
   safety_analysis/dfa
   safety_analysis/aou_requirements
   security_analysis/aou_requirements
