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

.. _lm_fork_vs_posix_spawn:

Detailed comparison of fork()+execve() vs posix_spawn (QNX-only)
================================================================

.. list-table::
   :header-rows: 1
   :widths: 20 30 30 30

   * - Property (config)
     - fork() + execve() (current)
     - posix_spawn() (QNX, new)
     - Difference
   * - **Executable / argv** (``process_command``)
     - ``access(X_OK)`` in the parent, then ``execve(executablePath, argv, envp)`` in the child
     - ``posix_spawn(argv[0], …, argv, envp)`` (``argv[0]`` is the executable path), no PATH lookup
     - Same result. Exec errors (``ENOENT``, ``EACCES``, …) are returned by ``posix_spawn()`` → ``kFail``, instead of making the child exit with ``EXIT_FAILURE``
   * - **Environment** (``environmental_variables``)
     - ``envp`` passed to ``execve()``
     - ``envp`` passed to ``posix_spawn()``
     - None
   * - **Comms fd** (``application_type`` ≠ Native)
     - Child: ``dup2(fd, 111)`` + ``fcntl(F_SETFD, 0)`` to clear ``FD_CLOEXEC``
     - File action ``posix_spawn_file_actions_adddup2(fd, 111)``. The dup'ed fd is not ``FD_CLOEXEC``
     - Same result. Native processes get no comms fd in either case
   * - **Process group** (implicit)
     - Child: ``setpgid(0, getpid())``
     - ``posix_spawnattr_setpgroup(0)`` + ``POSIX_SPAWN_SETPGROUP``
     - Same result
   * - **Signal dispositions / mask** (implicit)
     - Child: ``sigaction(SIG_DFL)`` for every signal except KILL/STOP, then ``pthread_sigmask(empty)``
     - ``posix_spawnattr_setsigdefault(all)`` + ``setsigmask(empty)`` (``POSIX_SPAWN_SETSIGDEF | SETSIGMASK``)
     - Same result. There is no window between fork and reset in which Launch Manager's handlers run in the child
   * - **uid / gid** (``sandbox.uid``, ``sandbox.gid``)
     - Child: ``setgid()`` → ``setgroups()`` → ``setuid()``
     - ``posix_spawnattr_setcred(uid, gid)`` + ``POSIX_SPAWN_SETCRED``
     - Same result. A failure makes ``posix_spawn()`` fail → ``kFail``, no process is created
   * - **Supplementary groups** (``sandbox.supplementary_group_ids``)
     - Child: ``setgroups(n, list)`` before ``setuid()``. Always called, so an empty list clears Launch Manager's groups
     - Launch Manager: ``setgroupspid(n, list, pid)`` on the held process. Always called
     - Same result. Applied by Launch Manager to another process instead of by the child to itself
   * - **Scheduling policy / priority** (``sandbox.scheduling_policy``, ``sandbox.scheduling_priority``)
     - Child: ``sched_setscheduler(0, …)``
     - Launch Manager: ``sched_setscheduler(pid, …)`` on the held process. The spawn attribute is deliberately not used, to avoid priority inversion while ``posix_spawn()`` waits for the child to initialize
     - Same priority: it is now clamped to ``sched_get_priority_min/max(policy)`` once at configuration load (with a warning) instead of in the forked child
   * - **CPU affinity** (``sandbox.affinity_mask``, optional)
     - Child: ``ThreadCtl(_NTO_TCTL_RUNMASK_GET_AND_SET_INHERIT)`` with runmask = inherit mask = configured mask (up to 64 CPUs)
     - ``pthread_spawnattr_setrunmask_np(mask)`` (64 bit) + ``POSIX_SPAWN_EXPLICIT_CPU``
     - Set at process creation instead of before exec. Threads created later should inherit the mask, as they do with the explicit inherit mask on the fork path *(to be confirmed)*
   * - **Resource limits** (``sandbox.rlimits``)
     - Child: ``setrlimit()`` for each limit, **before** dropping privileges
     - Launch Manager: ``prlimit(pid, …)`` for each limit on the held process
     - Same limits, but set by Launch Manager on another process instead of by the child on itself
   * - **RLIMIT_STACK** (in ``sandbox.rlimits``)
     - ``setrlimit()`` before ``execve()``. The new image's main thread stack follows the limit
     - Also ``posix_spawnattr_setstackmax(soft limit)`` + ``POSIX_SPAWN_SETSTACKMAX``, because the main thread stack already exists while the process is held
     - If the soft limit is ``RLIM_INFINITY`` or > 4 GiB, a warning is logged and the main thread gets the **default** stack size. The rlimit itself is still applied with ``prlimit()``
   * - **Working directory** (``working_dir``)
     - Child: ``chdir(working_dir)`` **after** ``setuid()``, so access is checked as the **target uid**
     - Launch Manager opens the directory (``O_RDONLY | O_DIRECTORY | O_CLOEXEC``) and passes the fd via ``posix_spawnattr_setcwd_np()`` + ``POSIX_SPAWN_SETCWD``, so access is checked as **Launch Manager's uid**
     - A directory the target uid cannot access was rejected before and is now accepted. However, the child fails to access any files if its credentials do not allow it.
   * - **Security policy** (``sandbox.security_policy``, optional)
     - Child: ``secpol_transition_type(nullptr, name, 1)`` after ``setuid()``/``chdir()``, just before ``execve()``. ``ENOTSUP`` is ignored
     - ``secpol_posix_spawnattr_settypeid(name, SECPOL_TYPE_NAME)`` (sets ``POSIX_SPAWN_SETTYPEID``). ``kOperationNotSupported`` is ignored
     - The process runs in its security type from the start, with no transition by the child. Launch Manager's policy must allow spawning into the type instead of the child's transition

Error handling and observable behavior
--------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 20 30 30

   * - Aspect
     - fork() + execve() (current)
     - posix_spawn() (QNX, new)
   * - Failure while preparing (attributes, ``open(working_dir)``, ``posix_spawn()`` itself)
     - Child exits with ``EXIT_FAILURE``. ``startProcess()`` already returned ``kSuccess``
     - ``startProcess()`` returns ``kFail``, ``pid = -1``, no process is created
   * - Failure while configuring the held process (scheduling, groups, rlimits)
     - n/a (all done in the child)
     - ``SIGKILL`` sent to the held process, ``startProcess()`` returns ``kSuccess``. Termination is reported through the normal path
   * - Termination status seen by Launch Manager after a setup failure
     - ``WIFEXITED``, exit code ``1``
     - ``WIFSIGNALED``, ``SIGKILL``
   * - Multiple failing settings
     - ``setSchedulingAndSecurity()`` tries all settings and logs every failure, then exits
     - Stops at the first failure
   * - Logging
     - ``signal_safe_log`` from the forked child (async-signal-safe only)
     - Regular ``LM_LOG_*`` from Launch Manager, with the executable and pid
   * - ``SIGCONT`` fails
     - n/a
     - Error logged and ``SIGKILL`` sent
   * - ``fflush(stdout)`` before start
     - Needed so buffered output is not duplicated
     - Not needed
   * - Order of application
     - signals → comms fd → rlimits → pgid → scheduling → affinity → gid → groups → uid → chdir → secpol → exec
     - At spawn: pgid, signals, affinity, uid/gid, stack max, cwd, security type, comms fd. While held: scheduling → groups → rlimits. Then ``SIGCONT``
