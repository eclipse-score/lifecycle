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

Glossary
========

.. glossary::
    Lifecycle Interface
      Interface to report :term:`readiness <Ready State>` of a process to the
      :term:`Launch Manager`.

    Component failure
      A state when the component has failed to reach its :term:`Ready Condition`, a running component has terminated abnormally or its supervision failed.

    UID
      User Identifier - a unique number assigned to each user on a Unix-like operating system.

    GID
      Group Identifier - a unique number assigned to each group on a Unix-like operating system.

    Working Directory
      The current directory from which a process is executed, also known as CWD (Current Working Directory).

    File Descriptor
      A handle used by a process to access files or other input/output resources.

    Procmgr
      Process Manager - a QNX system component that manages process creation and execution.

    ASLR
      Address Space Layout Randomization - a security technique that randomizes the memory layout of processes.

    QNX
      A real-time operating system commonly used in embedded systems.

    DSS
      Device Safe State - a safe operational state that a system can enter during failures.

    DAG
      Directed Acyclic Graph - a data structure used to represent dependencies between processes.

    Fallback Run Target
      A designated Run Target that the system can switch to in case the current active Run Target resp. one of its components fails.
