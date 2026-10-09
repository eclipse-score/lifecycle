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

.. _lifecycle_design_decisions:

Design Decisions
################

In-Process Health Monitor
=========================

.. dec_rec:: In-Process Health Monitor
   :id: dec_rec__lifecycle__in_process_health_monitor
   :status: accepted
   :version: 1
   :context: Architecture
   :decision: The Health Monitor is realized as a library linked into the monitored application.
   :consequences: Inter process communication is only required for the alive monitoring towards the Launch Manager and the configuration becomes simpler, at the cost of a harder safety argumentation.
   :affects: doc__lifecycle_module_architecture

The :term:`Health Monitor` is realized as a library linked into the monitored
application rather than as a separate monitoring process. The motivation and
the resulting trade-offs are the following.

Context
-------

The :term:`Health Monitor` provides the application health monitoring functions
(checkpoint supervision, deadline supervision and logical supervision) in a
similar fashion as the AUTOSAR `Platform Health Manager` (PHM). These functions
can either be linked into the monitored application or be placed in a separate
monitoring process.

Consequences
------------

- Inter process communication (IPC) only needed for the alive monitoring between :term:`Health Monitor` and :term:`Launch Manager`
- Easier configuration
    - The monitoring rules can be configured dynamically on demand basis
    - The monitoring can be started and stopped dynamically
- Debugging of problems possibly easier
    - Mapping of the events from a single process vs. the monitored application and the monitor

Alternatives Considered
-----------------------

Classical external process monitoring
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In a clasiscal external process monitoring, the application process only reports checkpoints via IPC to a separate monitoring daemon.
The monitoring daemon is configured with the supervisions that shall be checked on the reported checkpoints.

In this approach the safety argumentation is clear: The monitoring is completely done in a separate process.

Drawbacks of the selected solution over classical external process monitoring:

- Harder safety argumentation. The following chapter describes the issues and the possible solutions.

Justification for the Decision
------------------------------

As the :term:`Health Monitor` is linked as part of the monitored application, it raises the following concerns
with respect to safety:

- How can it be ensured, that the monitored application does not interfere with the monitoring functionality?
- How can it be ensured, that the :term:`Health Monitor` does not incorrectly report alive to the :term:`Launch Manager` when it has detected
  a supervision error?


These concerns are valid, but can be addressed using the following techniques:

- Hiding of the internal data from the user:
    - For example, if the monitoring is implemented in a thread, the thread ID must not be exposed to the calling application.
    - Hide the implementation for example with the pImpl-approach
- Protecting the memory of the library by using guard pages where the application memory is located, and protect it with mprotect()
   - possibly with a help of a custom allocator
- Protecting the data with a checksum and possibly a sequence counter
   - The internal data of the library can be checksum'ed every operation cycle, and by adding for example, a sequence
     counter (or some more complex mathematical function), further checkpoints for detecting misbehavior can be implemented
- Using a safe programming language, which does not allow a raw pointer access
- Testing the application with Valgrid etc.
- Redundant monitoring, if the self monitoring with above is not sufficient, an another library in another memory location can detect sporadic corruption of the other.


Launch Manager starts processes on a thread pool
================================================

.. dec_rec:: Thread Pool for process starting and stopping
   :id: dec_rec__lifecycle__thread_pool_process
   :status: accepted
   :version: 1
   :context: Architecture
   :decision: The Launch Manager uses a configurable thread pool to start and stop processes concurrently
   :consequences: Process lifecycle operations are organized around a configurable thread pool
   :affects: comp__lifecycle_launch_manager

The Launch Manager uses a configurable thread pool to start and stop processes concurrently.
Projects can adjust the pool size to meet their needs and reduce startup time.

Context
-------

The Launch Manager is responsible for starting and stopping system processes.
Large systems may require many processes to be started or stopped, with multiple operations able to run concurrently.
Fast startup is an important performance goal for the Launch Manager.

Consequences
------------

The Launch Manager's process lifecycle operations are organized around a configurable thread pool, whose size can be adapted to project requirements.

Launch Manager Alive Monitoring is polling-based
================================================

.. dec_rec:: Launch Manager Alive Monitoring is polling-based
   :id: dec_rec__lifecycle__lm_alive_polling
   :status: accepted
   :version: 1
   :context: Architecture
   :decision: The Launch Manager evaluates alive notifications periodically rather than as they arrive
   :consequences: Reduced CPU load, with failure detection delayed by up to one polling cycle
   :affects: comp__lifecycle_launch_manager

The Launch Manager polls IPC channels at a configurable interval, collecting alive notifications and evaluating alive supervisions.

Applications write notifications to an IPC buffer without waiting for the Launch Manager to process them.
This reduces the Launch Manager's CPU load compared with handling each notification as it arrives.

Context
-------

The Launch Manager supervises application processes by checking whether they send alive notifications at the expected frequency.
A system may have many application processes sending notifications at short intervals.

Consequences
------------

Because the Launch Manager evaluates notifications once per cycle instead of as they arrive, it uses less CPU to process them.
An alive supervision failure may therefore go undetected until the next polling cycle.
Configure the cycle time to balance the desired detection time against acceptable CPU load.


Flatbuffer Configuration
========================

.. dec_rec:: Flatbuffer Configuration
   :id: dec_rec__lifecycle__flatbuffer_config
   :status: accepted
   :version: 1
   :context: Architecture
   :decision: The Launch Manager uses binary configuration format to reduce startup time
   :consequences: Fast configuration loading
   :affects: comp__lifecycle_launch_manager

The Launch Manager uses FlatBuffers as its binary configuration format to reduce startup time.

Context
-------

At startup, the Launch Manager reads configuration files to determine which processes to start.
Loading this configuration is on the time-critical startup path.


Consequences
------------

The Launch Manager configuration must first be compiled into a binary format for the target.
Compared with a textual format such as JSON, the FlatBuffers binary configuration can be loaded faster, particularly for larger configuration files.