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

Known Limitations
*****************

Configuration
=============

Component
---------

* The sandbox parameters ``max_memory_usage`` and ``max_cpu_usage`` are
  currently not supported and are ignored.
* For ReadyCondition ``process_state:Terminated``, the mapping is only
  supported for Components that have at least one Component depending on them.
* The ``ready_recovery_action`` is deprecated in favour of
  ``activation_retry_attempts`` and ``activation_retry_delay_ms``.
  Its ``restart.number_of_attempts`` is converted to ``activation_retry_attempts``
  and ``restart.delay_before_restart_ms`` to ``activation_retry_delay_ms``.
* The ``activation_retry_delay_ms`` is currently not evaluated by the Launch Manager.
* The ``recovery_action`` only supports ``switch_run_target`` with the
  ``run_target`` set to ``fallback_run_target``.
* The ``ready_timeout_ms`` is used as the timeout until process state Running is
  reached, even in case the ReadyCondition is ``process_state:Terminated``.


Run Target
----------

* The parameter ``run_targets/<RunTarget>/transition_timeout_ms`` is currently not
  supported and is ignored.
* The ``recovery_action`` only supports ``switch_run_target`` with the
  ``run_target`` set to ``fallback_run_target``.
