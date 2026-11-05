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

Launch Manager Security Assumptions of Use
==========================================

.. document:: Launch Manager Component Security AoU
   :id: doc__launch_manager_comp_sec_aou
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :realizes: wp__requirements_comp_aou[version==1]

The integrator of the Launch Manager has to meet the AoUs on this page. AoUs that are relevant to
both safety and security are on :doc:`../safety_analysis/aou_requirements`; see
:need:`aou_req__launch_manager__config_protection` and
:need:`aou_req__launch_manager__ready_file`.

Component AoU
-------------

.. aou_req:: Own non-root identity per service
   :id: aou_req__launch_manager__sec_identity
   :reqtype: Non-Functional
   :security: YES
   :safety: QM
   :status: valid
   :version: 1

   In the Launch Manager configuration, the integrator shall give every long-running component its
   own uid and gid, which shall not be root and shall not be shared with another component. A
   component that runs as root shall be limited to a short-lived preparation step that ends before
   the services that depend on it start.

.. aou_req:: Supplementary groups only for allowed callers
   :id: aou_req__launch_manager__sec_groups
   :reqtype: Non-Functional
   :security: YES
   :safety: QM
   :status: valid
   :version: 1

   The integrator shall add a component to the group of another component only if the access control
   policy allows that component to call the other one. Every supplementary group in the Launch Manager
   configuration grants access to all resources that the group can access, for example sockets with
   group permissions.

.. aou_req:: Runtime directories prepared before services start
   :id: aou_req__launch_manager__sec_runtime_dirs
   :reqtype: Process
   :security: YES
   :safety: QM
   :status: valid
   :version: 1

   The Launch Manager does not create directories for components. Before a component starts, the
   integrator shall create the runtime and state directories it needs, with the owner and the
   permissions the component expects. This can be done by the image build or by a preparation
   component that the services depend on.

.. aou_req:: Privileges of a non-root Launch Manager
   :id: aou_req__launch_manager__sec_non_root_lm
   :reqtype: Non-Functional
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :tags: environment

   If the Launch Manager does not run as root, the integrator shall grant it the privileges that the
   operating system requires for changing the uid, gid and supplementary groups of the processes it
   starts, for example ``CAP_SETUID`` and ``CAP_SETGID`` on Linux, and no other privileges.

   .. note::
      A Launch Manager that runs as root is part of the trusted computing base of every component it
      starts.

.. aou_req:: Restricted access to the control service
   :id: aou_req__launch_manager__sec_control_access
   :reqtype: Non-Functional
   :security: YES
   :safety: QM
   :status: valid
   :version: 1

   In the mw::com configuration of the Launch Manager control service, the integrator shall set
   ``allowedConsumer`` to only the uids of the State Managers, ``allowedProvider`` to only the uid of
   the Launch Manager, and ``permission-checks`` to ``strict``.

   .. note::
      Any process that can call the service can change the active Run Target. With empty lists and
      ``file-permissions-on-empty``, the access is not restricted.

.. needextend:: c.this_doc() and is_external == False
   :+tags: lifecycle, launch_manager
