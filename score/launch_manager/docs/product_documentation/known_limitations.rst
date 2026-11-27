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

* The ``activation_retry_delay_ms`` is currently not evaluated by the Launch Manager.
  This also applies to the start attempts of a ``restart`` recovery action.
* The ``recovery_action`` only supports ``switch_run_target`` with the
  ``run_target`` set to ``fallback_run_target``.
* The case where a process does not terminate in response to SIGKILL is not yet correctly handled. See https://github.com/eclipse-score/lifecycle/issues/1079
* The ``recovery_action`` ``switch_run_target`` only supports the ``run_target``
  set to ``fallback_run_target``.
* There is no limit on the number of ``restart`` recovery actions over time. Each
  unexpected termination after reaching the Ready State triggers a new recovery with
  the full ``activation_retry_attempts`` budget, so a component that reliably reaches
  the Ready State and then terminates is restarted indefinitely.
* The ``ready_timeout_ms`` is used as the timeout until process state Running is
  reached, even in case the ReadyCondition is ``process_state:Terminated``.


Run Target
----------

* The parameter ``run_targets/<RunTarget>/transition_timeout_ms`` is currently not
  supported and is ignored.
* The ``recovery_action`` only supports ``switch_run_target`` with the
  ``run_target`` set to ``fallback_run_target``.
