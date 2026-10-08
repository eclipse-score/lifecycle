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

.. _life_statistics:

Verification Report
===================

.. document:: Lifecycle Verification Report
   :id: doc__lifecycle_verification_report
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: YES
   :realizes: wp__verification_module_ver_report
   :post_template: module_verification_report

   Verification report for the Lifecycle module. This report is generated based on the module verification plan and the module verification work product (:need:`wp__verification_module_ver_report`), and covers all components of the module.



The following additional tables are used for checking traceability completeness.


Tracability of Feature Requirements to (valid) Component Requirements
---------------------------------------------------------------------

Feature requirements without any derived component requirements are highlighted in red.

.. dropdown:: Show component traceability table
   :animate: fade-in

   .. needtable:: Valid lifecycle feature requirements and traced component requirements
      :filter: type == "feat_req" and "feat__lifecycle" in satisfied_by and status == "valid"
      :style: table
      :types: feat_req
      :columns: id;title;derived_from_back;status
      :colwidths: 15,25,45,15
      :sort: title
      :class: verification-trace-table

Traceability of (valid) Feature Requirements to Feature Architecture
--------------------------------------------------------------------

Feature requirements without any linked feature architecture elements are highlighted in red.

.. dropdown:: Show architecture traceability table
   :animate: fade-in

   .. needtable:: Valid lifecycle feature requirements and linked architecture elements
      :filter: type == "feat_req" and "feat__lifecycle" in satisfied_by and status == "valid"
      :style: table
      :types: feat_req
      :columns: id;title;fulfils_back;status
      :colwidths: 15,25,45,15
      :sort: title
      :class: verification-trace-table

Traceability of (valid) Component Requirements to Component Architecture
------------------------------------------------------------------------

Valid component requirements without any linked component architecture elements are highlighted in red.

.. dropdown:: Show component architecture traceability table
   :animate: fade-in

   .. needtable:: Valid component requirements and linked component architecture elements
      :filter: type == "comp_req" and ("comp__health_monitor" in satisfied_by or "comp__lifecycle_launch_manager" in satisfied_by) and status == "valid"
      :style: table
      :types: comp_req
      :columns: id;title;fulfils_back;status
      :colwidths: 15,25,45,15
      :sort: title
      :class: verification-trace-table

.. raw:: html

   <style>
      table.verification-trace-table tr:has(td:nth-child(3) > p:empty) > td {
       background-color: #f8d7da;
       color: #842029;
   }
   </style>

