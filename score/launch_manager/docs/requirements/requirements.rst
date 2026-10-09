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

.. document:: Launch Manager Requirements
   :id: doc__launch_manager_requirements
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: YES
   :realizes: wp__requirements_comp[version==1]

.. note:: 
    Requirements which are not planned to be implemented in the version 1.0 of S-CORE are set to status **invalid**.


Component Launch Manager Requirements
#####################################

Components
==========

.. note::
   TODO all requirements that are not under the Process heading shall 
   be reworded to speak in terms of components.

.. comp_req:: Invalid dependency
    :id: comp_req__launch_man__consistent_dependencies
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__process_ordering[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall reject an inconsistent definition of set of executables dependencies.


Activating Components
---------------------

.. comp_req:: Configuration of component activation timeout
    :id: comp_req__launch_man__conf_comp_active_tout
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support configuring a timeout value that
    defines the maximum time allowed for a component to reach its
    :term:`Ready State`.

.. comp_req:: Component activation timeout
    :id: comp_req__launch_man__comp_activate_tout
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    If a component does not reach its :term:`Ready State` within the configured
    timeout, the :term:`Launch Manager` shall consider the component activation
    attempt as failed.


Deactivating Components
-----------------------

.. comp_req:: Fast shutdown
    :id: comp_req__launch_man__fast_shutdown_support
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__termination_dependency[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support fast shutdown by terminating itself
    without considering the started :term:`Processes <Process>`.

    .. note::
        Fast shutdown allows the :term:`Launch Manager` to terminate its own process **normally**, without waiting for the started :term:`Processes <Process>`.
        **Use case:** This is relevant when the system needs to restart quickly, so it shuts down the launch manager in ordinary manner, but ignores the child processes.

.. comp_req:: Normal shutdown
    :id: comp_req__launch_man__launch_manager_shutdown
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__termination_dependency[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support normal shutdown by terminating all
    process in the dependency order.

.. comp_req:: Launch Manager shutdown
    :id: comp_req__launch_man__launcher_exit_shutdown
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__termination_dependency[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall exit after performing shutdown operation by
    stopping all the :term:`Processes <Process>` it owns in the dependency order when requested.

.. comp_req:: Dropping process responsibility
    :id: comp_req__launch_man__drop_supervsion
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__running_processes[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support to dropping all surveillance
    and failure reaction activities of :term:`Processes <Process>`.

    .. note::
        **Use case:** When the :term:`Launch Manager` shuts down, selected :term:`Processes <Process>` can be kept alive to continue their execution without interruption.

.. comp_req:: Coordination stop dependency
    :id: comp_req__launch_man__stop_order_spec
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__termination_dependency[version==1]
    :status: invalid
    :tags: unclear
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall permit the stop order of non-dependent processes to be specified.


.. comp_req:: Dangling dependency
    :id: comp_req__launch_man__stop_process_dependents
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__termination_dependency[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall be able to stop a process when all it's dependents are stopped if specified in the set of executables.


Conditional Launching
---------------------

.. comp_req:: Configuration of component readiness conditions
    :id: comp_req__launch_man__conf_of_comp_ready_cond
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support configuration of conditions that
    shall be met before the component is considered to have reached its
    :term:`Ready State`.

.. comp_req:: Dependency based startup order
    :id: comp_req__launch_man__dep_based_startup_order
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall start a component only after all its
    :term:`dependencies <Dependency (between components)>` have successfully reached their :term:`Ready State`.


Ready Conditions
^^^^^^^^^^^^^^^^

.. comp_req:: Ready Condition - OS Process State
    :id: comp_req__launch_man__rc_os_state
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support a :term:`Ready Condition` that is
    satisfied when the configured binary is launched.

.. comp_req:: Ready Condition - Lifecycle Interface
    :id: comp_req__launch_man__rc_lifecycle
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support a :term:`Ready Condition` that is
    satisfied when :term:`Component` notifies the :term:`Lifecycle Interface`.

.. comp_req:: Ready Condition - File state 
    :id: comp_req__launch_man__rc_file_state
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support a :term:`Ready Condition` that is
    satisfied when a configured file path exists or does not exist.

.. comp_req:: Condition check based on at least one dependency
    :id: comp_req__launch_man__check_dependency_exec
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide a method to check if at least one dependency has been executed.

.. comp_req:: Condition check for each SWC its dependencies
    :id: comp_req__launch_man__define_swc_dependencies
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide a way to define for each :term:`SWC` (Software Components), its dependencies.

Processes
---------
Launching Processes
^^^^^^^^^^^^^^^^^^^

.. comp_req:: Forward process information
    :id: comp_req__launch_man__process_input_output
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support to pass the output of one or
    multiple :term:`Processes <Process>` as input arguments to another process.

    .. note::
        This is a similar concept to piping in shell scripting, where the output of one process can be used as the input to another.

.. comp_req:: Multiple instance of executable
    :id: comp_req__launch_man__multi_start_support
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__running_processes[version==1]
    :status: invalid
    :tags: unclear
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall permit an executable to be launched more than once.


Process Configuration
"""""""""""""""""""""
.. 
    Why does everything map here to feat_req__lifecycle__custom_cond_support? 
    When looking on this requirement it seems it is related to Control interface API.

.. comp_req:: Handling process args
    :id: comp_req__launch_man__process_launch_args
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__launch_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching a process with a
    given set of arguments.

.. comp_req:: Process user, group IDs support
    :id: comp_req__launch_man__uid_gid_support
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching a process with a
    given :term:`UID`/:term:`GID` (user name/Group Identifier).

.. comp_req:: Process priority support
    :id: comp_req__launch_man__launch_priority_support
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching a process with a
    given priority.

.. comp_req:: CWD support
    :id: comp_req__launch_man__cwd_support
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching a process with a
    given :term:`Working Directory`.

.. comp_req:: Launching terminal
    :id: comp_req__launch_man__terminal_support
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching a terminal or a
    session leader.

.. comp_req:: Standard handle redirection
    :id: comp_req__launch_man__std_handle_redir
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for stdin, stdout, stderr
    redirection.

.. comp_req:: Non-root support
    :id: comp_req__launch_man__secpol_non_root
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support to be started with security
    policy as non-root.

.. comp_req:: Configurable amount of retries on startup
    :id: comp_req__launch_man__retries_configurable
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support restarting the process 
    a configurable amount of times if the process fails to reach its
    :term:`Ready State`.

.. comp_req:: Process capability support
    :id: comp_req__launch_man__capability_support
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching :term:`Processes <Process>`
    with configured OS-specific capabilities and privileges.

.. comp_req:: File descriptor inheritance support
    :id: comp_req__launch_man__fd_inheritance
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching a process with
    given :term:`File Descriptor` inheritance restrictions.


.. comp_req:: Security policy support
    :id: comp_req__launch_man__support_secpol_type
    :reqtype: Functional
    :security: YES
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching a process with a
    given security policy.

.. comp_req:: Supplementary group support
    :id: comp_req__launch_man__supplementary_groups
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching a process with a
    given set of supplementary groups.

.. comp_req:: Scheduling support
    :id: comp_req__launch_man__scheduling_policy
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching a process with
    certain scheduling policy.

.. comp_req:: CPU runmask support
    :id: comp_req__launch_man__runmask_support
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching a process with a
    given runmask.

.. comp_req:: ASLR support
    :id: comp_req__launch_man__aslr_support
    :reqtype: Functional
    :security: YES
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching process with
    :term:`ASLR` (Address Space Layout Randomization).

.. comp_req:: Resource limit support
    :id: comp_req__launch_man__process_rlimit_support
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching a process with 
    a set of system resource limits (rlimit) which are defined by the 
    POSIX-Standard (IEEE Std 1003.1). I.e. RLIMIT_CORE, RLIMIT_CPU,
    RLIMIT_DATA, RLIMIT_FSIZE, RLIMIT_NOFILE, RLIMIT_STACK and RLIMIT_AS.

.. comp_req:: Process detach from parent support
    :id: comp_req__launch_man__detach_parent_process
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__sandbox_options[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching a process to
    detach from parent.

    .. note::
        Detaching from the parent process is also known as creating a daemon process.
        **Use case:** There might be processes which need to continue running independently of the launch manager.

.. comp_req:: Launching processes in parallel
    :id: comp_req__launch_man__launch_parallel
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__launch_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for launching :term:`Processes <Process>`
    in parallel.


Terminating Processes
^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Stop timeout
    :id: comp_req__launch_man__configurable_timeout
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__process_termination[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for configurable timeout
    :term:`Interval` to wait for the process to be stopped.

.. comp_req:: Configurable delay between SIGTERM and SIGKILL
    :id: comp_req__launch_man__time_to_wait_config
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__process_termination[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall offer a configuration of the time to wait before
    SIGKILL is sent. In case "0" is stated, the SIGKILL shall be sent immediately.

.. comp_req:: Shutdown signal handling
    :id: comp_req__launch_man__shutdown_signal
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__process_termination[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall implement a shutdown by sending a SIGTERM to
    the process. In case the process does not terminate itself, a SIGKILL shall be sent.


Containers
----------

.. comp_req:: Runtime configuration compliance
    :id: comp_req__launch_man__runtime_config_compat
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__config_file_support[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The launch manager shall provide modular configuration files support for configurations coming from `OCI runtime configuration<https://github.com/opencontainers/runtime-spec/blob/v1.2.0/config.md>`.


Run Targets
===========

.. comp_req:: Launching run target
    :id: comp_req__launch_man__start_named_run_target
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__run_target_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall be able to start a named :term:`Run Target`.

.. comp_req:: Run target to component dependencies
    :id: comp_req__launch_man__rt_comp_dep
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support to
    define a :term:`Run Target <Run Target>`
    :term:`dependency <Dependency (between run targets)>` to :term:`Components <Component>`.

.. comp_req:: Run target to run target dependencies
    :id: comp_req__launch_man__rt_rt_dep
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support to
    define a :term:`Run Target <Run Target>`
    :term:`dependency <Dependency (between run targets)>` to another
    :term:`Run Target`.

.. comp_req:: Configuration of Run Target activation timeout
    :id: comp_req__launch_man__conf_rt_active_tout
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support configuration of the maximum time
    an activation of a Run Target can take.

.. comp_req:: Run Target activation timeout
    :id: comp_req__launch_man__rt_activate_tout
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__conditional_startup[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall consider a :term:`Run Target` activation
    as failed if the activation exceeds the maximum configured time.

.. comp_req:: Process state
    :id: comp_req__launch_man__process_state_comm
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__process_ordering[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall have a means for the launched :term:`Processes <Process>`
    to communicate a state, which represents the launched processes' internal state,
    to the launcher.

.. comp_req:: Switch between run targets
    :id: comp_req__launch_man__switch_run_targets
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__run_target_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall be able to switch between different :term:`run targets <Run target>`.


Alive Monitoring
================

.. comp_req:: Process state notification
    :id: comp_req__launch_man__ext_monitor_notify
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__monitor_processes[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for external monitors to get
    notified on process life status.

.. comp_req:: Monitoring and recovery: adopted process monitoring
    :id: comp_req__launch_man__monitoring_processes
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__monitor_processes[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for monitoring adopted
    :term:`Processes <Process>`.

    .. note::
        An adopted process is a process that was not originally launched by the :term:`Launch Manager`.
        **Use case:** There might be processes which are needed to start very early during bootup and 
        are therefore launched by the system before the :term:`Launch Manager` takes control.


Recovery Actions
----------------

.. comp_req:: Component monitoring during startup
    :id: comp_req__launch_man__failure_detect_startup
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__monitor_processes[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall be able to detect :term:`Component
    failure` during startup of the :term:`Component`. I.e. before reaching its
    :term:`Ready State`.

.. comp_req:: Component monitoring during runtime
    :id: comp_req__launch_man__failure_detect_runtime
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__monitor_processes[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall be able to detect :term:`Component
    failure` during runtime of the :term:`Component`. I.e. after reaching its
    :term:`Ready State`.

.. comp_req:: Monitoring and recovery: recovery wait time
    :id: comp_req__launch_man__configurable_wait_time
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support configuring the time to wait before
    reactivating a failed :term:`Component`.

    .. note::
       IDK if we want this actually...

.. comp_req:: Component recovery action failure
    :id: comp_req__launch_man__ra_comp_failed
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall consider a :term:`Component` recovery
    action as failed if the :term:`Component` does not reach its
    :term:`Ready State`.

    .. note::
       Does it?

.. comp_req:: Component recovery action failure escalation
    :id: comp_req__launch_man__ra_comp_failed_esc
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall perform the :term:`Run Target` recovery
    action configured for the :term:`Run Target` that contains the failed
    :term:`Component` if a :term:`Component` recovery action fails.


.. comp_req:: Run target recovery action failure
    :id: comp_req__launch_man__ra_rt_failed
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall consider a :term:`Run Target` recovery action
    as failed if the activation of the configured :term:`Run Target` fails.

    .. note::
       Maybe not? If your RT-RA is to switch RT does the wording mean the
       RT-RA failed?

.. comp_req:: Run target recovery action failure escalation
    :id: comp_req__launch_man__ra_rt_failed_esc
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall perform the configured fallback recovery
    action if a :term:`Run Target` recovery action fails.

.. comp_req:: Fallback recovery action failure
    :id: comp_req__launch_man__ra_fallback_failed
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall consider the fallback recovery action as
    failed if the activation of the :term:`Fallback Run Target` fails.

.. comp_req:: Stopping the servicing of the watchdog
    :id: comp_req__launch_man__ra_fallback_failed_watchdog
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall stop the servicing of the
    :term:`Watchdog` if the fallback recovery action fails.

Component Recovery Actions
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Recovery Action - component failure reactivate the component
    :id: comp_req__launch_man__ra_comp_reactivate
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support reacting to a
    :term:`Component failure`
    by reactivating the failed :term:`Component`.

.. comp_req:: Recovery Action - component failure switch Run Target
    :id: comp_req__launch_man__ra_comp_switch_rt
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support reacting to a
    :term:`Component failure`
    by switching to a configured :term:`Run Target`.

.. comp_req:: Recovery Action - component failure stopping the component
    :id: comp_req__launch_man__ra_comp_stop
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: invalid
    :tags: unclear
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support reacting to a
    :term:`Component failure`
    by stopping the failed :term:`Component`.

.. comp_req:: Recovery Action - component failure replacing component
    :id: comp_req__launch_man__ra_comp_replace
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: invalid
    :tags: unclear
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support reacting to a
    :term:`Component failure`
    by stopping the failed :term:`Component` and then starting a configured
    :term:`Component` instead.

.. comp_req:: Recovery Action - component failure switch to fallback run target
    :id: comp_req__launch_man__ra_comp_fallback
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support reacting to a
    :term:`Component failure`
    by switching to the :term:`Fallback Run Target`.

    .. note::
        Shall this be configurable on the Component or shall it follow the
        escalation chain?


.. comp_req:: Recovery Action - component failure stopping the servicing of the watchdog
    :id: comp_req__launch_man__ra_comp_watchdog
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support reacting to a
    :term:`Component failure`
    by stopping the servicing of the :term:`Watchdog`.

    .. note::
        Shall this be configurable on the Component or shall it follow the
        escalation chain?


Run Target Recovery Actions
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Recovery Action - run target failure switch Run Target
    :id: comp_req__launch_man__ra_rt_switch_rt
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support reacting to a
    :term:`Run Target` activation failure
    by switching to a configured :term:`Run Target`.

.. comp_req:: Recovery Action - run target failure switch to fallback run target
    :id: comp_req__launch_man__ra_rt_fallback
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support reacting to a
    :term:`Run Target` activation failure
    by switching to the :term:`Fallback Run Target`.

    .. note::
        Shall this be configurable on the Run Target or shall it follow the
        escalation chain?

.. comp_req:: Recovery Action - run target failure stopping the servicing of the watchdog
    :id: comp_req__launch_man__ra_rt_watchdog
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__recovery_action_support[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support reacting to a
    :term:`Run Target` activation failure
    by stopping the servicing of the :term:`Watchdog`.

    .. note::
        Shall this be configurable on the Run Target or shall it follow the
        escalation chain?


Watchdog
========

.. comp_req:: Launch manager external watchdog notification
    :id: comp_req__launch_man__lm_ext_watchdog_notify
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__lm_self_health_check[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall trigger a notification to an external
    :term:`Watchdog` for each successful self monitoring test execution.

.. comp_req:: Launch manager external watchdog notification - failed test
    :id: comp_req__launch_man__lm_ext_wdg_failed_test
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__lm_self_health_check[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall not trigger an external :term:`Watchdog`
    notification if an internal health check failed.

.. comp_req:: Launch manager external monitoring configuration
    :id: comp_req__launch_man__lm_ext_watchdog_cfg
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__lm_self_health_check[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support configuring the :term:`Interval` of
    the internal health check executions.


Logging
=======

.. comp_req:: Logging slog2 and file support
    :id: comp_req__launch_man__slog2_logging
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__logging_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall support OS specific logging facilities to analyze the early
    boot sequence.

.. comp_req:: Logging state transitions
    :id: comp_req__launch_man__process_logging_support
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__logging_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide support for logging process launches,
    :term:`Processes <Process>` exit/recovery, internal tasks, and interaction with external monitor.

.. comp_req:: Logging timestamp
    :id: comp_req__launch_man__log_timestamp
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__logging_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` logs shall contain timestamp information.

.. comp_req:: Logging DAG
    :id: comp_req__launch_man__dag_logging_controlif
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__logging_support[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall provide the possibility to log the :term:`DAG`
    in a human readable format, triggered via :term:`Control Interface`.

.. comp_req:: Configuration dependency view
    :id: comp_req__launch_man__dependency_visu
    :reqtype: Functional
    :security: NO
    :safety: QM
    :derived_from: feat_req__lifecycle__deps_visualization[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall have the means to log the current dependencies in a format that can be visualized when requested.

Configuration file
==================

.. comp_req:: Configuration file support
    :id: comp_req__launch_man__modular_config_support
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__config_file_support[version==1]
    :status: invalid
    :tags: not_planned
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The launch manager shall provide modular configuration file support to configure process attributes.

.. comp_req:: Global process properties
    :id: comp_req__launch_man__central_default_defines
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__component_group_config[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall be able to centrally define defaults for specific properties for the set of executables.

.. comp_req:: Lazy check of configured commands
    :id: comp_req__launch_man__lazy_check
    :reqtype: Functional
    :security: NO
    :safety: ASIL_B
    :derived_from: feat_req__lifecycle__component_group_config[version==1]
    :status: valid
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall check availability of executables in the filesystem only when the executable shall required to be executed.

.. comp_req:: Configuration Verification tool
    :id: comp_req__launch_man__offline_config_valid
    :reqtype: Functional
    :security: NO
    :safety: QM
    :derived_from: feat_req__lifecycle__deps_visualization[version==1]
    :status: invalid
    :tags: unclear
    :version: 1
    :satisfied_by: comp__lifecycle_launch_manager

    The :term:`Launch Manager` shall have a means to validate the configuration offline.

.. needextend:: c.this_doc() and is_external == False and "__launch_manager__" in id
   :+tags: lifecycle, launch_manager

