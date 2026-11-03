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

Launch Manager Safety Assumptions of Use
========================================

.. document:: Launch Manager Component AoU
   :id: doc__launch_manager_comp_aou
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_comp_aou[version==1]

The integrator of the Launch Manager has to meet the AoUs on this page. Some of them come from
the AoUs of the Launch Manager's own dependencies, which only the integrator can meet (see
eclipse-score/lifecycle#873). The others come from running the Launch Manager in an integration.

Component AoU
-------------

.. aou_req:: Protected configuration and executable
   :id: aou_req__launch_manager__config_protection
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall protect the Launch Manager executable and its configuration binary (the
   ``-c`` argument, or ``etc/launch_manager_config.bin`` relative to the working directory):

   * Both shall be stored on a read-only filesystem whose integrity is protected, owned by root and
     not writable by any other user, so that no other software can modify or replace them, neither
     before nor while the Launch Manager reads them.
   * Only trusted parties, such as the init system, shall start the Launch Manager or set its
     arguments and working directory. The configuration path shall be a POSIX path.

   .. note::
      The configuration defines the uid, gid, supplementary groups and environment of every
      component. The Launch Manager checks only the structure and the schema version of the file, not
      its integrity or its origin.

.. aou_req:: No lifecycle client use outside main and no console logging
   :id: aou_req__launch_manager__logging
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall not use the lifecycle client APIs (``LifeCycleManager``, ``run_application``,
   ``report_running()``, ``score_mw_lifecycle_report_running()``) during static initialization or
   destruction, that is, before ``main()`` is entered or after it has returned.

   The mw::log configuration of the Launch Manager and of applications using the lifecycle client
   shall not enable console logging in production, and production builds shall not set
   ``--//config:use_cout_log=True``.

.. aou_req:: Error reaction on lifecycle client errors
   :id: aou_req__launch_manager__error_reaction
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall check every ``score::Result`` returned by ``ILmControl`` and the return value
   of ``score_mw_lifecycle_report_running()``, and shall react to an error by moving the system to a
   safe state. The integrator shall also define the safe state that follows when the Launch Manager
   exits with ``EXIT_FAILURE``.

.. aou_req:: mw::com configuration of the control service
   :id: aou_req__launch_manager__com_config
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   In every mw::com configuration that contains the Launch Manager control service, the integrator
   shall:

   * define the same service type (event, methods and IDs);
   * set ``maxSubscribers`` of the activation result event to at least the number of ``ILmControl``
     instances on the target;
   * set ``numberOfSampleSlots`` to at least ``maxSubscribers`` × 4, because ``ILmControl`` always
     subscribes with 4 samples;
   * size the method queues for the expected load;
   * set the ASIL levels of the process and the instance according to the safety classification of the
     Launch Manager and of the State Manager;
   * not enable event oversubscription.

.. aou_req:: Platform constraints of mw::com
   :id: aou_req__launch_manager__com_platform
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :tags: environment

   For safety-relevant deployments, the integrator shall:

   * run the Launch Manager and the State Manager only on the operating systems that mw::com
     supports for safety;
   * store the mw::com configurations on a filesystem qualified for the highest ASIL in use;
   * run processes of different ASIL under different uids;
   * build the Launch Manager and every State Manager that links ``ILmControl`` with identical
     compiler settings that affect data layout;
   * ensure that no other software accesses memory or message-passing endpoints managed by LoLa,
     or creates shared-memory segments whose names start with ``lola``.

.. aou_req:: State Manager robustness against the control path
   :id: aou_req__launch_manager__sm_robustness
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The State Manager shall not hold ``ILmControl`` in static or global storage. Using timeouts or
   external supervision, it shall make sure that none of these cases violates a safety goal:

   * the control service is not found;
   * an activation result notification is lost, late or spurious, including results lost because more
     than four of them settle before the State Manager reads them;
   * a method call blocks or gets no answer;
   * the Launch Manager terminates, which ``ILmControl`` does not report.

.. aou_req:: Alive notifications from supervised components
   :id: aou_req__launch_manager__alive_reporting
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall configure a component with the application type ``Reporting_And_Supervised``
   or ``State_Manager`` only if that component uses the lifecycle client to report running and to
   send alive notifications. All other components shall be configured as ``Native`` or ``Reporting``.

   .. note::
      If a component of one of these types sends no alive notifications, alive supervision expires
      within seconds, and the recovery action stops its Run Target. This was observed with a State
      Manager that does not link the lifecycle client.

.. aou_req:: Ready file of a file_state ready condition
   :id: aou_req__launch_manager__ready_file
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1

   For each component that has a ``file_state`` ready condition, the integrator shall make sure that:

   * the watched file is in a directory that only that component or root can write to;
   * the watched file does not exist before the component is started, for example because the
     directory is cleaned before the start.

   .. note::
      The Launch Manager treats the component as ready as soon as the file exists. If a file is left
      over from an earlier run, or another process creates it, the dependent components start before
      the service is ready.

.. aou_req:: No dependency on standard input
   :id: aou_req__launch_manager__no_stdin
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   Components started by the Launch Manager shall not depend on standard input. In particular, they
   shall not exit or change behavior when standard input reaches end-of-file. Child processes inherit
   the standard input of the Launch Manager.

.. aou_req:: Scheduling of the Launch Manager
   :id: aou_req__launch_manager__exec_priority
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall run the Launch Manager with a scheduling priority and CPU budget that let it
   detect a failure and complete the configured recovery action within the fault tolerant time
   interval of the safety goals it serves.

   .. note::
      :need:`comp_req__launch_man__failure_detect` and :need:`comp_req__launch_man__alive_failure`
      only hold if the Launch Manager itself is scheduled in time.

.. aou_req:: External watchdog for the Launch Manager
   :id: aou_req__launch_manager__ext_watchdog
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The integrator shall connect the Launch Manager to an external watchdog that moves the system to
   a safe state when the Launch Manager stops servicing it.

   .. note::
      The Launch Manager stops servicing the watchdog on
      :need:`comp_req__launch_man__lm_ext_wdg_failed_test` and
      :need:`comp_req__launch_man__ra_fallback_failed_esc`.

.. needextend:: c.this_doc() and is_external == False
   :+tags: lifecycle, launch_manager
