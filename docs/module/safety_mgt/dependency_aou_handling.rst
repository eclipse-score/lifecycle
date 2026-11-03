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

Assumptions of Use of Dependencies
##################################

.. document:: Lifecycle Handling of Dependency Assumptions of Use
   :id: doc__lifecycle_dependency_aou_handling
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: YES
   :realizes: wp__module_safety_manual[version==1]

This page records, for every valid Assumption of Use (AoU) of the modules Lifecycle depends on, whether Lifecycle
fulfils it or forwards it to the user or system integrator of Lifecycle
(`eclipse-score/lifecycle#873 <https://github.com/eclipse-score/lifecycle/issues/873>`_).

- **Fulfilled**: Lifecycle code, configuration or design satisfies the AoU. The listed component is the one that
  will get a ``fulfils`` link to the AoU once the verdict is agreed.
- **Forwarded**: Lifecycle cannot satisfy the AoU alone. It moves to the Lifecycle safety manual as a Lifecycle AoU;
  the text given here is a proposal.
- **Not applicable**: Lifecycle does not use the element the AoU constrains.
- **Open**: needs a decision by a Lifecycle maintainer; the question is given.

Scope: the dependencies used by non-test code, taken from the Bazel targets Lifecycle references
(baselibs ``concurrency``, ``containers_rust``, ``filesystem``, ``flatbuffers``, ``mw/log``; communication ``mw/com``),
plus the OS module and platform AoUs. AoU IDs are given as plain text, because the needs of baselibs are not yet an
external needs source of this documentation. mw::com AoUs were checked against ``score_communication`` 0.6.0; those
added in 0.6.0 have no ``aou_req__communication`` ID yet and are listed by their TRLC name.

.. note::

   This is a draft. Verdicts were derived from the Lifecycle sources at the commit this page was added with and still
   need review by the Lifecycle maintainers.

Summary
=======

.. list-table::
   :header-rows: 1
   :widths: 30 14 14 16 10 10

   * - Source
     - Fulfilled
     - Forwarded
     - Not applicable
     - Open
     - Total
   * - baselibs (concurrency, containers, filesystem, flatbuffers, mw::log)
     - 9
     - 5
     - 12
     - 1
     - 27
   * - communication (mw::com / LoLa)
     - 18
     - 13
     - 2
     - 3
     - 36
   * - OS module
     - 1
     - 2
     - 0
     - 3
     - 6
   * - Platform
     - 0
     - 16
     - 9
     - 2
     - 27
   * - **Total**
     - 28
     - 36
     - 23
     - 9
     - 96

Open Points
===========

- ``aou_req__flatbuffers__verify_version``: Maintainer question: is LM's own schema_version check (after Verifier) accepted as equivalent, or must LM adopt the score::flatbuffers version check (major/minor + 4-char identifier, present in baselibs v0.2.16) before accessing buffer data?
- ``aou_req__communication__14``: Maintainer question: is losing an activation_result event acceptable (QM-only interface), or must LM react (e.g. fallback run target / safe state) on Allocate()/Send() failure? If acceptable, document rationale; else implement reaction.
- ``aou_req__communication__29``: Maintainer question: is unbounded mw::com execution time on the LM control-loop thread acceptable for LM's timing-relevant duties (alive supervision, watchdog servicing), or is it covered by aou_req__launch_manager__ext_watchdog?
- ``aou_req__communication__4``: Maintainer question: what ASIL does the LM control interface (LmControlService) have? If ASIL-B, example config must be ASIL-B; if QM, document that the control path is QM. Then forward the config text.
- ``aou_req__os__cxx_preference``: Maintainer question: is clock_gettime a C-lib or OS (POSIX) API for this AoU, and is mixing it with std::chrono for time measurement acceptable? Otherwise unify on std::chrono (or document a deviation). No other obvious mix (no malloc/strcpy/atoi).
- ``aou_req__os__rst0001``: Maintainer with QNX safety manual access must read restriction RST-0001 and decide whether LM (process launch/privileges) or HM is affected; then link fulfils or forward.
- ``aou_req__os__safe_fctn_use``: Maintainer must check every OS call in osal/ and process_launcher.cpp against the QNX safe function list (esp. fork in a multithreaded process, acl\_\*, secpol, ioctl) and record the analysis; Linux build is non-safety reference.
- ``aou_req__platform__no_exceptions``: Currently NOT fulfilled. Maintainer must decide: remove catch blocks and link safecpp/aborts_upon_exception (then FULFILLED), or document a deviation. HM C++ wrapper has no catch; Rust has no exceptions (panic policy to be stated).
- ``aou_req__platform__no_heap_alloc_after_init``: 'should' AoU: maintainer must either remove runtime allocations (memory pool for HM deadlines, LM state machine) or justify the deviation in the Lifecycle safety concept.

baselibs (concurrency, containers, filesystem, flatbuffers, mw::log)
====================================================================

.. list-table::
   :header-rows: 1
   :widths: 24 10 12 34 20

   * - AoU
     - Verdict
     - Component
     - Evidence
     - Forwarded AoU / note
   * - ``aou_req__flatbuffers__verify_version`` FlatBuffers buffer version check before access
     - Open
     - Launch Manager
     - LM does not use the baselibs version check (version_reader / buffer_version, file identifier); lm_flatcfg.fbs has no file_identifier. It checks its own int32 schema_version after structural verification (flatbuffer_config_loader.cpp:48-62, 98).
     - 
   * - ``aou_req__filesystem__path_validation`` Path Validation
     - Forwarded
     - Launch Manager
     - Config path comes from the command line (-c) or default 'etc/launch_manager_config.bin' (run.cpp:111-118) and is passed unvalidated to LoadBuffer. Only the integrator controls the start command and filesystem.
     - The system integrator shall ensure that the configuration path passed to the Launch Manager (-c option or default etc/launch_manager_config.bin) is valid and refers to a trusted location. (Could be merged into aou_req__launch_manager__config_integrity.)
   * - ``aou_req__flatbuffers__access_control`` FlatBuffers access control
     - Forwarded
     - Launch Manager
     - LM reads its config binary via score::flatbuffers::LoadBuffer (flatbuffer_config_loader.hpp:45); file protection (permissions, dm-verity) is a system configuration matter LM cannot enforce.
     - The system integrator shall ensure access control and manipulation prevention for the Launch Manager FlatBuffers configuration file (e.g. read-only/dm-verity protected filesystem). Partially covered by aou_req__launch_manager__config_integrity; consider adding a security AoU. Note: HM (Rust) does not use flatbuffers; alive_impl lists flatbufferscpp in BUILD (alive/src/details/BUILD:37) but has no call sites.
   * - ``aou_req__flatbuffers__data_integrity`` FlatBuffers data integrity
     - Forwarded
     - Launch Manager
     - LM performs only structural Verifier check (flatbuffer_config_loader.cpp:86-90) and no checksum; integrity relies on storage. Matches draft aou_req__launch_manager__config_integrity (certified filesystem).
     - aou_req__launch_manager__config_integrity
   * - ``aou_req__log__console_logging`` Console Logging
     - Forwarded
     - Launch Manager
     - LM backend is chosen by the deployment's mw::log config (test/example ecu_logging_config.json use logMode kConsole); build flag //config:use_cout_log switches LM to std::cout logging (common/BUILD:50-63). HM uses log_rust; stdout_logger only in tests.
     - The system integrator shall not configure console logging (mw::log logMode kConsole, or the //config:use_cout_log build flag) for the Launch Manager and the lifecycle client libraries in production.
   * - ``aou_req__log__no_static_log`` Logging During the C++ Static Storage construction and destruction.
     - Forwarded
     - Launch Manager
     - LM daemon: main.cpp only calls run(); logger is a function-local static ref (common/log.hpp _getLmLogger), no static objects that log. But client lib LifeCycleManager ctor/dtor log (lifecyclemanager.cpp:40,67) so app code creating it statically would violate.
     - Users of the lifecycle client library shall not create score::mw::lifecycle::LifeCycleManager or other lifecycle client objects with static storage duration. Note: Daemon part is fulfilled; forwarded part is only for client libs running in user processes.
   * - ``aou_req__filesystem__error_handling`` Error Handling
     - Fulfilled
     - Launch Manager
     - Only score::filesystem::Path is used (config_loader.hpp:18,53). The only file op, LoadBuffer(path), has its result checked: flatbuffer_config_loader.hpp:61-65 (mapOsError); run.cpp:142-147 exits with LM_LOG_FATAL on failure.
     - Note: Path construction itself returns no error.
   * - ``aou_req__filesystem__low_level_use_only`` Low-Level Use Only
     - Fulfilled
     - Launch Manager
     - Filesystem lib used only for the Path type required by score::flatbuffers::LoadBuffer (flatbuffer_config_loader.hpp:43-46; BUILD target configuration:config_loader). No other filesystem API used.
     - Note: Arguably NOT_APPLICABLE as LM does no direct filesystem manipulation.
   * - ``aou_req__filesystem__performance`` Performance
     - Fulfilled
     - Launch Manager
     - Config is loaded once at daemon start before any timed supervision starts (run.cpp:141-147, before AliveMonitor/ProcessGroupManager creation); no timing assumption/timeout around the load.
     - 
   * - ``aou_req__filesystem__supported_path_formats`` Supported Path Formats
     - Fulfilled
     - Launch Manager
     - LM targets only Linux/QNX (POSIX); default path is POSIX relative path (run.cpp:111). A user-supplied -c path is also passed through.
     - Note: If maintainers want to be strict, the -c path aspect can be covered by the forwarded path_validation AoU.
   * - ``aou_req__filesystem__thread_safety`` Thread Safety
     - Fulfilled
     - Launch Manager
     - Path/LoadBuffer used only in run.cpp:141-142 on the main thread before ProcessGroupManager/ThreadPool threads are created; no concurrent filesystem access via the library.
     - 
   * - ``aou_req__flatbuffers__verify_structure`` FlatBuffers buffer verification before access
     - Fulfilled
     - Launch Manager
     - score/launch_manager/src/daemon/src/configuration/details/flatbuffer_config_loader.cpp:86-90: ::flatbuffers::Verifier + VerifyLaunchManagerConfigBuffer, returns InvalidFormat before GetLaunchManagerConfig (line 92).
     - 
   * - ``aou_req__log__private_or_undocumented_apis`` Undocumented or Private APIs
     - Fulfilled
     - Launch Manager
     - Only \\public mw::log APIs used: CreateLogger, Logger, LogInfo/LogError, LogStream, LogString (logging.h/logger.h/log_types.h:118-120 tagged \\public). No log::detail(s) usage found by grep in score/ or examples/.
     - Note: HM uses log_rust (score_log), not mw::log C++ API; mw::log::rust::StdoutLoggerBuilder only in HM test log_init.cpp and examples.
   * - ``aou_req__log__relying_on_logs`` Applications relying on logs verification/qualification/case.
     - Fulfilled
     - Launch Manager
     - LM/client/HM only emit logs, never read them to drive behavior; integration tests (tests/integration/\*) assert on process/run-target state, not log content (no log grepping found).
     - Note: Also applies to comp__health_monitor (log_rust only emitted). Application-side verification remains with each application per AoU wording.
   * - ``aou_req__log__unsupported_contexts`` Unsupported Contexts
     - Fulfilled
     - Launch Manager
     - LM signal handler only sets an atomic (process_group_manager.cpp:32-35); client handles SIGTERM via sigwait thread, not async handler (lifecyclemanager.cpp:142-150); post-fork child code uses signal_safe_log, not mw::log (process_launcher.cpp:66,121,133). LogStreams are temporaries.
     - Note: No interrupt handlers in Lifecycle.
   * - ``aou_req__concurrency__coop_cancellation`` Cooperative Cancellation Handling
     - Not applicable
     - 
     - AoU targets tasks submitted to the baselibs executor (score/concurrency:executor). Lifecycle never uses it: only uses concurrency:condition_variable (termination_waiter.hpp:76, lm_control_impl.hpp:486), :notification (completion_slot.hpp:64) and /future. LM ThreadPool is its own (daemon/src/common/concurrency/thread_pool.hpp:34).
     - Note: Even so, LM honours stop_token in its own waits (process_monitor.cpp:79, termination_waiter.cpp:46, lm_control_impl.hpp:260), so a fulfils link to comp__lifecycle_launch_manager would also be defensible if baselibs considers the AoU component-wide.
   * - ``aou_req__concurrency__memory_management`` Memory Management on Task Submission Failure
     - Not applicable
     - 
     - Concerns task submission into executor/ThreadPool with bounded memory resource. git grep in lifecycle: no concurrency::Executor/ThreadPool/Task/SimpleTask usage; deps limited to condition_variable, notification, future.
     - 
   * - ``aou_req__concurrency__thread_pool_sizing`` Thread Pool Sizing
     - Not applicable
     - 
     - Baselibs ThreadPool not used. The ThreadPool in process_group_manager.cpp:186 is Lifecycle's own class (score/launch_manager/src/daemon/src/common/concurrency/thread_pool.hpp), not score::concurrency::ThreadPool.
     - 
   * - ``aou_req__concurrency__thread_safety`` Thread Safety
     - Not applicable
     - 
     - AoU covers state shared between executor tasks; Lifecycle submits no tasks to the baselibs executor. Used primitives (InterruptibleConditionalVariableBasic, Notification) are paired with own std::mutex (termination_waiter.cpp:46, lm_control_impl.hpp:260).
     - Note: If baselibs means the AoU for all concurrency primitives, LM's guarded usage would support a FULFILLED link for comp__lifecycle_launch_manager.
   * - ``aou_req__containers__bounds_checking`` Index Bounds Checking
     - Not applicable
     - 
     - AoU belongs to C++ comp__baselibs_containers (score/containers). Lifecycle has no dep on @score_baselibs//score/containers and no score::containers include; only HM Rust uses containers_rust (FixedCapacityVec, worker.rs:16, health_monitor.rs:23).
     - Note: containers_rust (comp__baselibs_containers_rust) defines no AoUs on origin/main; Rust indexing is bounds-checked anyway.
   * - ``aou_req__containers__capacity_management`` Check Capacity
     - Not applicable
     - 
     - C++ containers not used (see bounds_checking). For the Rust FixedCapacityVec the push error is handled anyway: health_monitor.rs:298-300 (push().is_err() -> error!).
     - 
   * - ``aou_req__containers__ele_type_requirements`` Element Type Requirements
     - Not applicable
     - 
     - C++ containers/intrusive list not used by Lifecycle; only Rust containers_rust:containers in score/health_monitor/src/rust/BUILD:17 (Rust type system enforces element bounds).
     - 
   * - ``aou_req__containers__element_lifetime`` Element Lifetime and Ownership
     - Not applicable
     - 
     - C++ containers not used; Rust containers_rust ownership is borrow-checked. No score/containers dep in any Lifecycle BUILD.
     - 
   * - ``aou_req__containers__iterator_validity`` Iterator Validity
     - Not applicable
     - 
     - C++ containers not used; Rust iterator invalidation is prevented by the borrow checker.
     - 
   * - ``aou_req__containers__memory_management`` Memory Resource Management
     - Not applicable
     - 
     - C++ containers not used (no @score_baselibs//score/containers dep). Rust FixedCapacityVec sized at construction (health_monitor.rs:346 FixedCapacityVec::new(num_monitors)).
     - 
   * - ``aou_req__containers__state_verification`` Container State Verification
     - Not applicable
     - 
     - C++ containers not used; only Rust containers_rust in HM.
     - 
   * - ``aou_req__containers__thread_safety`` Thread Safety
     - Not applicable
     - 
     - C++ containers not used; Rust Send/Sync enforces cross-thread access rules for containers_rust.
     - 

communication (mw::com / LoLa)
==============================

.. list-table::
   :header-rows: 1
   :widths: 24 10 12 34 20

   * - AoU
     - Verdict
     - Component
     - Evidence
     - Forwarded AoU / note
   * - ``aou_req__communication__14`` Check for nullptr on Allocate()
     - Open
     - Launch Manager
     - score/launch_manager/src/daemon/src/control/control_provider.hpp:209-215 checks Allocate() result (mw::com returns Result, not nullptr) but on failure only logs and returns; the activation_result event is dropped, no safe-state transition.
     - Note: Named CheckForAllocationError in communication 0.6.0 (Allocate() error instead of nullptr).
   * - ``aou_req__communication__29`` No guarantee on execution time
     - Open
     - Launch Manager
     - LM calls Skeleton Allocate()/Send() synchronously from Graph::finalizeTransitionSuccess on its control loop (score/launch_manager/src/daemon/src/control/control_provider.hpp:207-229, graph.cpp:234); client wait_for_connection documented to possibly exceed timeout (ilm_control.hpp:107).
     - Forward to users: the State Manager shall not assume bounded execution time for ILmControl calls.
   * - ``aou_req__communication__4`` Correctly configured ASIL Level
     - Open
     - Launch Manager
     - Example/test mw::com config sets asil-level QM for both instances and global (score/launch_manager/docs/product_documentation/examples/mw_com_config.json (= tests/utils/environments/mw_com_config.json)), while LM requirements and aou_req__launch_manager__control_if_access are ASIL_B.
     - The system integrator shall configure asil-level of the LmControlService instance according to the ASIL of the Launch Manager control interface and its State Manager.
   * - ``aou_req__communication__1`` Monotonic Semi-Dynamic Memory Allocation
     - Forwarded
     - Launch Manager
     - Shared-memory sizing of LmControlService comes from the integrator's mw::com config; Lifecycle only ships an example (score/launch_manager/docs/product_documentation/examples/mw_com_config.json (= tests/utils/environments/mw_com_config.json): numberOfSampleSlots 8, maxSubscribers 1). No shm-size handling in Lifecycle code.
     - The system integrator shall configure the mw::com/LoLa configuration of the LmControlService instance (LaunchManager/StateManager/Instance) such that enough shared memory is available for all LoLa allocations. Note: mw::com is used only by LM: daemon skeleton ControlProvider (control_provider.hpp, created run.cpp:177-182) and client lib //score/launch_manager/src/lm_control (proxy LmControlImpl). Health Monitor does not use mw::com.
   * - ``aou_req__communication__10`` Config on a safe filesystem
     - Forwarded
     - Launch Manager
     - LM daemon and lm_control clients read mw_com_config.json at runtime (LM uses mw::com default path, no explicit InitializeRuntime in daemon). Draft aou_req__launch_manager__config_integrity covers only the LM config (launch_manager_config.bin).
     - Extend aou_req__launch_manager__config_integrity: the integrator shall also store the mw::com configuration used by the Launch Manager and its control clients on a filesystem certified for the highest safety level. Note: Map to aou_req__launch_manager__config_integrity (extend its scope to mw_com_config.json).
   * - ``aou_req__communication__12`` No guarantee in availability of services
     - Forwarded
     - Launch Manager
     - Client lib handles absence: async StartFindService (score/launch_manager/src/lm_control/src/details/lm_control_impl.hpp:126), calls return kCommunicationError until found, wait_for_connection with timeout. Daemon: OfferService failure -> LM_LOG_FATAL + EXIT_FAILURE (run.cpp:184-188).
     - The user of ILmControl (State Manager) shall ensure that unavailability of the Launch Manager control service (kCommunicationError / wait_for_connection timeout) does not violate a safety goal. Note: Lifecycle reports unavailability but cannot decide the safety reaction of the State Manager.
   * - ``aou_req__communication__13`` No notification on termination of producer
     - Forwarded
     - Launch Manager
     - Producer of LmControlService is the LM daemon. LM termination is not notified to the State Manager proxy; LM failure detection relies on the external watchdog.
     - Covered by aou_req__launch_manager__ext_watchdog; additionally: the State Manager shall not rely on the LM control interface for safety reactions if the Launch Manager terminates. Note: Map to aou_req__launch_manager__ext_watchdog.
   * - ``aou_req__communication__2`` Correctly Configured Maximum Number of Subscribers
     - Forwarded
     - Launch Manager
     - maxSubscribers is integrator config; example sets maxSubscribers 1 on LaunchManager/StateManager/Instance (score/launch_manager/docs/product_documentation/examples/mw_com_config.json (= tests/utils/environments/mw_com_config.json)). Lifecycle code does not set/validate it.
     - The system integrator shall configure maxSubscribers of the ActivationResult event of the LmControlService instance to the number of State Manager (ILmControl) clients that subscribe.
   * - ``aou_req__communication__3`` Correctly configured maximum number of maximum elements per subscriber
     - Forwarded
     - Launch Manager
     - Client subscribes with max 4 samples (kActivationResultSampleCount, score/launch_manager/src/lm_control/src/details/lm_control_impl.hpp:338); numberOfSampleSlots is integrator config (example 8, score/launch_manager/docs/product_documentation/examples/mw_com_config.json (= tests/utils/environments/mw_com_config.json)).
     - The system integrator shall configure numberOfSampleSlots of the ActivationResult event so that each configured subscriber can hold the 4 samples requested by ILmControl.
   * - ``aou_req__communication__30`` Usage of configuration "oversubscription"
     - Forwarded
     - Launch Manager
     - Oversubscription is a config option; Lifecycle example config does not enable it (score/launch_manager/docs/product_documentation/examples/mw_com_config.json (= tests/utils/environments/mw_com_config.json)); grep for oversubscription/enforceMaxSamples in Lifecycle is empty.
     - The system integrator shall not enable oversubscription for the LmControlService instance, or ensure the State Manager tolerates activation_result loss.
   * - ``aou_req__communication__31`` Same compiler settings for provider and consumer side
     - Forwarded
     - Launch Manager
     - Skeleton (LM daemon) and proxy (lm_control linked into the State Manager) are compiled separately from score/launch_manager/src/lm_control/src/details/lm_control_service.h; the State Manager build is outside Lifecycle.
     - The integrator shall build the Launch Manager and all ILmControl users (State Manager) with identical ABI-relevant compiler settings.
   * - ``aou_req__communication__34`` Correctly configured events/fields per service type
     - Forwarded
     - Launch Manager
     - LM and State Manager may use separate mw::com configs; Lifecycle ships one example covering both (score/launch_manager/docs/product_documentation/examples/mw_com_config.json (= tests/utils/environments/mw_com_config.json), integration.rst:66-72).
     - The system integrator shall ensure the LmControlService service type definition (events/methods/IDs) is identical in the mw::com configurations of the Launch Manager and all State Managers.
   * - ``aou_req__communication__7`` No guarantees for notifications
     - Forwarded
     - Launch Manager
     - Client mitigates missed notifications: awaitNotificationRegistration + drain on register (score/launch_manager/src/lm_control/src/details/lm_control_impl.hpp:300-315,213-221); State Manager can poll get_active_run_target.
     - The State Manager shall not rely solely on activation_result notifications for safety-relevant decisions and shall use get_active_run_target() to confirm the active run target.
   * - ``aou_req__communication__8`` Checking for possible message overflow
     - Forwarded
     - Launch Manager
     - activation_result with 4 samples/subscriber; skeleton Send failure only logged (score/launch_manager/src/daemon/src/control/control_provider.hpp:221-226); method queueSize 1 in example config. Overflow loss cannot be prevented by LM alone.
     - Same as aou 7: the State Manager shall tolerate loss of activation_result events (e.g. via get_active_run_target()).
   * - ``aou_req__communication__9`` Different user for ASIL and QM processes
     - Forwarded
     - Launch Manager
     - LM applies configured uid/gid per launched process; choice of uids is in the integrator's LM config. LM's own uid is chosen by the integrator.
     - Extend aou_req__launch_manager__os_privileges or add: the integrator shall configure different user ids for processes of different ASIL (incl. Launch Manager vs. QM State Manager/clients). Note: Map to aou_req__launch_manager__os_privileges (partially).
   * - ``aou_req__communication__11`` No static context support
     - Fulfilled
     - Launch Manager
     - Skeleton: ControlProvider is a local unique_ptr in run.cpp:177-182 (no static). Proxy: ILmControl::Create returns heap unique_ptr (ilm_control.hpp:99); grep finds no static/global ControlProvider/LmControl instances.
     - Users of ILmControl (State Manager) shall not create or hold ILmControl instances in static storage (static/global objects). Note: LM side fulfilled; client lib side depends on the user where ILmControl is stored, so also forward the text to State Manager users (safety manual).
   * - ``aou_req__communication__15`` One producer only one AllocateePtr
     - Fulfilled
     - Launch Manager
     - score/launch_manager/src/daemon/src/control/control_provider.hpp:209-221 handleActivationResult allocates one AllocateePtr and immediately moves it into Send() in the same function; invoked from Graph::finalizeTransitionSuccess (graph.cpp:218-235) on the LM control loop.
     - Note: Medium: assumes finalizeTransitionSuccess is never called concurrently (single LM control-loop thread); confirm.
   * - ``aou_req__communication__16`` No Copy-Send() while holding AllocateePtr.
     - Fulfilled
     - Launch Manager
     - Only zero-copy Send(std::move(AllocateePtr)) is used (score/launch_manager/src/daemon/src/control/control_provider.hpp:221); no Send(const&) in Lifecycle.
     - 
   * - ``aou_req__communication__17`` None reentrant methods per event instance
     - Fulfilled
     - Launch Manager
     - Proxy: event API calls serialized by sample_processing_mutex\_ (score/launch_manager/src/lm_control/src/details/lm_control_impl.hpp:398 onActivationResult); Subscribe/SetReceiveHandler under mutex\_ in onServiceFound; Unsubscribe in dtor after discovery_scope\_.Expire(). Skeleton: Allocate/Send only from LM control loop.
     - Note: Finding: if a user ActivationCallback calls register_run_target_activation_callback() from inside the callback, onActivationResult re-locks non-recursive sample_processing_mutex\_ -> self-deadlock/UB. Not a LoLa reentrancy, but worth a usage note or fix.
   * - ``aou_req__communication__18`` Skeleton alive while its AllocateePtr being used
     - Fulfilled
     - Launch Manager
     - AllocateePtr is a local inside handleActivationResult and is sent before return (score/launch_manager/src/daemon/src/control/control_provider.hpp:209-221); skeleton\_ is a member of the same ControlProvider object, which outlives graph callbacks (deinitialize() before control_provider destruction, run.cpp:199-203).
     - 
   * - ``aou_req__communication__19`` Event Subscription active while holding SamplePtr
     - Fulfilled
     - Launch Manager
     - SamplePtrs exist only inside the GetNewSamples callback (forwardSample takes const&, copies fields to the user callback) (score/launch_manager/src/lm_control/src/details/lm_control_impl.hpp:415-418,452-465); Unsubscribe only in destructor (score/launch_manager/src/lm_control/src/details/lm_control_impl.hpp:170-173).
     - 
   * - ``aou_req__communication__20`` Non-Terminating callbacks
     - Fulfilled
     - Launch Manager
     - FindService handler lambdas are noexcept (score/launch_manager/src/lm_control/src/details/lm_control_impl.hpp:122-128); user ActivationCallback exceptions are caught in forwardSample (score/launch_manager/src/lm_control/src/details/lm_control_impl.hpp:461-475); skeleton method handlers call noexcept handlers (score/launch_manager/src/daemon/src/control/control_provider.hpp:115-118,165-168).
     - Note: Receive-handler lambda ({LC} SetReceiveHandler) and skeleton handler lambdas are not declared noexcept; std::mutex::lock may in theory throw std::system_error. Consider marking noexcept. User callbacks must not throw (ilm_control.hpp:66) but are guarded anyway.
   * - ``aou_req__communication__21`` Valid callbacks while proxy alive
     - Fulfilled
     - Launch Manager
     - Callbacks capture \`this\` of the object owning the proxy/skeleton. Proxy dtor: StopFindService, discovery_scope\_.Expire(), activation_result.Unsubscribe() (unsets handler) before members die (score/launch_manager/src/lm_control/src/details/lm_control_impl.hpp:153-174). ControlProvider is non-movable, owns skeleton\_ (score/launch_manager/src/daemon/src/control/control_provider.hpp:100-104).
     - 
   * - ``aou_req__communication__22`` Quality of data is dependent on producer
     - Fulfilled
     - Launch Manager
     - Skeleton process is the LM daemon (ASIL_B requirements, score/launch_manager/docs/requirements/requirements.rst); request validation (unknown run target, unsupported mode) in score/launch_manager/src/daemon/src/control/control_provider.hpp:130-160; responses/events filled from graph state.
     - Optionally forward: data quality of requests sent to LM is the responsibility of the State Manager (proxy side) - covered by aou_req__launch_manager__control_if_access for authorization. Note: Conditional on LM being qualified to the ASIL of its consumers; note example mw::com config declares the instance QM (see aou 4).
   * - ``aou_req__communication__23`` Validity of pointer on LoLa pointer
     - Fulfilled
     - Launch Manager
     - No pointers into sample/method memory escape: method responses read via const& within scope and copied out (score/launch_manager/src/lm_control/src/details/lm_control_impl.hpp:195-201,240-251); samples copied into the user callback by value (score/launch_manager/src/lm_control/src/details/lm_control_impl.hpp:463); skeleton writes via Get() before Send (score/launch_manager/src/daemon/src/control/control_provider.hpp:217-221).
     - 
   * - ``aou_req__communication__24`` LoLa Memory only accessed through LoLa
     - Fulfilled
     - Launch Manager
     - Lifecycle code never maps LoLa memory; LM's own shm is '/lm_running_report_ipc' (process_launcher.cpp:249). LM sets FD_CLOEXEC on LoLa fds after OfferService so launched children don't inherit them (score/launch_manager/src/daemon/src/control/control_provider.hpp:241-264, workaround communication#1064).
     - Platform-level: the integrator shall ensure no other process accesses LoLa shared memory (relies on OS access rights). Note: Fulfilled for Lifecycle's own code; system-wide part is platform/integrator level.
   * - ``aou_req__communication__25`` No shared memory allocation in namespace lola
     - Fulfilled
     - Launch Manager
     - Only shm created by Lifecycle code is '/lm_running_report_ipc' via shm_open_anon (process_launcher.cpp:249, osal/details/\*/shm_open_anon.cpp); no 'lola\*' segment names in Lifecycle.
     - Note: Rest of the system: platform-level / integrator.
   * - ``aou_req__communication__26`` LoLa specific QNX Messaging End-Points only accessed through LoLa
     - Fulfilled
     - Launch Manager
     - grep for name_attach/ChannelCreate/ConnectAttach in score/, examples/, tests/ finds nothing; LoLa QNX IPC is used only through mw::com APIs (lm_control, control_provider).
     - Note: System-wide part (other processes) is platform-level.
   * - ``aou_req__communication__28`` unsupported data-types
     - Fulfilled
     - Launch Manager
     - Transmitted types are POD structs of uint8 enums and FixedString<128> (std::array<char>) (score/launch_manager/src/lm_control/src/details/lm_control_service.h:39-107, ilm_control.hpp:37); no std::variant/map.
     - 
   * - ``aou_req__communication__5`` Only LoLa supported types
     - Fulfilled
     - Launch Manager
     - Only trivially copyable POD types: uint8 enums, FixedString (std::array<char,129>) in score/launch_manager/src/lm_control/src/details/lm_control_service.h:39-107; method payloads same.
     - Note: Same evidence as aou 28.
   * - ``aou_req__communication__6`` No APIs from Implementation Namespace
     - Fulfilled
     - Launch Manager
     - grep 'com::impl' / 'mw/com/impl' in score/, examples/, tests/ empty; only public API used (score/mw/com/types.h, AsProxy/AsSkeleton, InstanceSpecifier, SamplePtr, MethodReturnTypePtr).
     - 
   * - ``aou_req__communication__27`` aragen not safe
     - Not applicable
     - Launch Manager
     - No aragen usage (grep 'aragen' empty); the LmControlService interface is hand-written (score/launch_manager/src/lm_control/src/details/lm_control_service.h:118-147).
     - Note: Removed in communication 0.6.0.
   * - ``aou_req__communication__32`` Event or Field reception via GenericProxy needs specific care
     - Not applicable
     - Launch Manager
     - No GenericProxy/GenericProxyEvent usage in Lifecycle (grep empty); only typed AsProxy/AsSkeleton.
     - 
   * - ``NoCreationOfServiceElements`` (0.6.0)
     - Fulfilled
     - Launch Manager
     - Events and methods exist only as members of LmControlService (score/launch_manager/src/lm_control/src/details/lm_control_service.h:131-143).
     - 
   * - ``NoMoveOfServiceElements`` (0.6.0)
     - Fulfilled
     - Launch Manager
     - Only the whole skeleton is moved (score/launch_manager/src/daemon/src/control/control_provider.hpp:69); no event or method member is moved.
     - 
   * - ``ArtifactsRequireManualReview`` (0.6.0)
     - Forwarded
     - Launch Manager
     - LmControlService is hand-written and reviewed in Lifecycle; the mw::com configuration is deployment-specific.
     - The system integrator shall review the mw::com configuration of LmControlService against the service definition.

OS module
=========

.. list-table::
   :header-rows: 1
   :widths: 24 10 12 34 20

   * - AoU
     - Verdict
     - Component
     - Evidence
     - Forwarded AoU / note
   * - ``aou_req__os__cxx_preference`` C++ Library Preference
     - Open
     - Launch Manager, Health Monitor
     - Timing mixes C and C++: clock_gettime(CLOCK_MONOTONIC) in process_group_manager/details/component.cpp:254,278, alive_monitor/details/timers/Timers_OsClock.cpp:29, osal/details/posix/semaphore.cpp:60 vs std::chrono in ~59 LM/HM files.
     - 
   * - ``aou_req__os__rst0001`` RST-0001
     - Open
     - Launch Manager
     - Placeholder AoU ("see QNX OsSafetyManual") in score docs/modules/os/docs/requirements/aou_req_qnx.rst:43-51; content not available in repos. LM uses QNX-specific APIs in osal/details/qnx/ (ThreadCtl, secpol, shm_open O_ANON, setgroups).
     - 
   * - ``aou_req__os__safe_fctn_use`` Safe OS Function Use
     - Open
     - Launch Manager, Health Monitor
     - ASIL_B LM uses fork()+execve (process_launcher.cpp:201,398), setuid/setgid/setgroups, sched_setscheduler, setrlimit, kill, shm_open/mmap, acl\_\*, secpol_transition_type, ThreadCtl, /dev watchdog ioctl (WatchdogImpl.cpp:151). No check vs. OS safe function list found.
     - 
   * - ``aou_req__os__minimal_privileges`` Minimal Process Privileges
     - Forwarded
     - Launch Manager
     - LM needs setuid/setgid/setgroups/sched_setscheduler/setrlimit/secpol_transition_type/ThreadCtl runmask (process_launcher.cpp:64,324,341,350,357; osal/details/qnx/security_policy.cpp:22, set_affinity.cpp:41). Own IPC shm restricted via ACL to one uid (alive_monitor/details/ipc/IpcServer.hpp:170-182).
     - aou_req__launch_manager__os_privileges ("...with no further privileges"); plus aou_req__launch_manager__control_if_access for the control channel. Integrator must also configure minimal sandbox (uid/gid/capabilities/secpol) for every launched component. Note: Privilege grant is a deployment decision; LM partly fulfils for its own IPC (ACL on shm) which could be linked via fulfils from comp__lifecycle_launch_manager. Consider extending the AoU to the sandbox config of launched processes.
   * - ``aou_req__os__nonroot`` No Root Privileges
     - Forwarded
     - Launch Manager
     - No privilege drop / procmgr_ability / capability code in LM (grep for root, procmgr_ability, cap_set: none). Config schema allows sandbox.uid minimum 0 (config_schema/launch_manager.schema.json:321-325), so root children are configurable.
     - Proposed: The system integrator shall start the Launch Manager with only the specific abilities/capabilities listed in aou_req__launch_manager__os_privileges instead of root, and shall not configure uid 0 for any launched component. Note: Could alternatively be strengthened in LM (reject uid 0 in offline validation, drop unused abilities after start). Maintainer should confirm LM can run non-root on QNX/Linux; current draft os_privileges does not exclude root.
   * - ``aou_req__os__public_api`` OS Public API Use
     - Fulfilled
     - Launch Manager, Health Monitor
     - OS access only via POSIX/public headers (unistd, sched, sys/mman, sys/acl, signal, semaphore) and documented QNX APIs isolated in osal/details/qnx/ (ThreadCtl sys/neutrino.h, secpol/secpol.h, _syspage_ptr num_cores.cpp:14,23); baselibs score/os wrappers. HM: Rust std + libc crate.
     - Note: Fulfils link should come with a requirements-analysis note; _syspage_ptr is documented public QNX API but reviewer should confirm.

Platform
========

.. list-table::
   :header-rows: 1
   :widths: 24 10 12 34 20

   * - AoU
     - Verdict
     - Component
     - Evidence
     - Forwarded AoU / note
   * - ``aou_req__platform__no_exceptions`` Avoidance of Exceptions
     - Open
     - Launch Manager, Health Monitor
     - LM production code handles exceptions: run.cpp:206 catch(...), lm_control_impl.hpp:141,146,467,473, alive.cpp:65, AliveMonitorImpl.cpp:53,57, AliveWorkerFactory.cpp:63,85,136, WatchdogImpl.cpp:79, log.hpp:91. No dep on safecpp/aborts_upon_exception, no -fno-exceptions.
     - 
   * - ``aou_req__platform__no_heap_alloc_after_init`` Avoidance of heap allocations after initialization
     - Open
     - Launch Manager, Health Monitor
     - HM allocates at runtime: health_monitor/src/rust/deadline/ffi.rs:22-34 ("For now, Deadlines are kept allocated on heap individually", TODO memory pool); protected_memory.rs:17 TODO. LM uses std::vector/std::string at runtime; FixedSizeVector only in alive_monitor.
     - 
   * - ``aou_req__platform__bug_report`` SW-platform integration bug reporting
     - Forwarded
     - 
     - Addressed to the system integrator; platform-wide, already in the platform safety manual.
     - Covered at platform level; no Lifecycle-specific AoU needed (optionally reference in safety_manual.rst).
   * - ``aou_req__platform__error_reaction`` Error Reaction
     - Forwarded
     - Launch Manager, Health Monitor
     - Lifecycle APIs report errors via score::Result: ilm_control.hpp:99-185 (activate_run_target, get_active_run_target, ...). LM itself consumes baselibs/mw::com Result types (18 deps on score/result).
     - Proposed: Applications using lm_control, lifecycle_client or the Health Monitor API shall evaluate the returned errors and perform an appropriate reaction (e.g. report to State Manager / reach safe state). Note: Platform-level AoU already reaches users; Lifecycle as consumer of baselibs/mw::com must also comply - confirm by code review (no evidence of systematic violations found).
   * - ``aou_req__platform__external_health_management`` External Health Management
     - Forwarded
     - Launch Manager
     - LM services an external watchdog (watchdog/details/WatchdogImpl.cpp:151 WDIOC_KEEPALIVE) and stops on self-check failure; draft AoU explicitly refines this platform AoU.
     - aou_req__launch_manager__ext_watchdog
   * - ``aou_req__platform__flow_monitoring`` Program Flow Monitoring
     - Forwarded
     - Health Monitor
     - Health Monitor provides deadline/logic/heartbeat monitors; draft AoU explicitly refines this platform AoU.
     - aou_req__health_monitor__instrumentation Note: LM daemon itself as safety app: its own supervision is via self health check + external watchdog; maintainer may justify in LM safety concept.
   * - ``aou_req__platform__hardware_safety`` Safe HW platform
     - Forwarded
     - 
     - System/HW-level mechanisms (memory protection, HW self test, external supervision); LM cannot influence.
     - Covered at platform level; LM relies on external supervision via aou_req__launch_manager__ext_watchdog.
   * - ``aou_req__platform__integration_safety_anomaly`` Integrator safety anomaly reporting
     - Forwarded
     - 
     - Addressed to system integrator; platform-wide.
     - Covered at platform level; no Lifecycle-specific AoU needed.
   * - ``aou_req__platform__levels`` Integration levels
     - Forwarded
     - 
     - Addressed to supplier and system integrator; platform-wide.
     - Covered at platform level.
   * - ``aou_req__platform__no_mixed_asil`` No mixed ASIL
     - Forwarded
     - Health Monitor
     - comp__lifecycle_launch_manager and comp__health_monitor are ASIL_B (component_architecture.rst:34/44); LM daemon deps (baselibs, mw/log, mw/com) are ASIL_B. HM is a library running inside the application process.
     - Proposed: The application process that links the Health Monitor shall be developed to at least ASIL_B (the highest ASIL in the process), since QM code in the same process can corrupt the Health Monitor. Note: LM daemon side can be FULFILLED (single ASIL_B process) - maintainer to confirm no QM code is linked into the daemon (e.g. QM log backends).
   * - ``aou_req__platform__os_safety_functions`` OS safety functions
     - Forwarded
     - Launch Manager
     - LM depends on the listed OS safety functions: process loading/starting (fork/execve), privilege restriction (setuid/secpol), priorities (sched_setscheduler), IPC shm, monotonic time.
     - Covered at platform level (integrator selects OS); aou_req__launch_manager__os_privileges and __exec_priority refine it for LM.
   * - ``aou_req__platform__posix_operating_system`` POSIX Operating System
     - Forwarded
     - 
     - LM is implemented on POSIX APIs (posix osal in osal/details/posix, linux/qnx variants).
     - Covered at platform level; no Lifecycle-specific AoU needed.
   * - ``aou_req__platform__process_isolation`` Process Isolation
     - Forwarded
     - Launch Manager
     - LM's freedom from interference w.r.t. supervised apps relies on OS process isolation (only shm with ACL shared; IpcServer.hpp:170-182). Arch checklist references this AoU (docs/features/lifecycle/architecture/chklst_arc_inspection.rst:166).
     - Covered at platform level; consider referencing it in Lifecycle safety_manual.rst as an assumption of LM/HM FFI argument.
   * - ``aou_req__platform__safety_integration`` Safety integration
     - Forwarded
     - 
     - Addressed to system integrator; platform-wide.
     - Covered at platform level.
   * - ``aou_req__platform__safety_matching`` Safety matching
     - Forwarded
     - 
     - Addressed to system integrator; platform-wide.
     - Covered at platform level.
   * - ``aou_req__platform__success_expected`` Success Expected
     - Forwarded
     - Launch Manager, Health Monitor
     - Lifecycle APIs return score::Result (ilm_control.hpp:99-185). LM itself checks most OS/baselibs returns but e.g. process_info_node.cpp:122 casts clock_gettime result to void.
     - Same as proposed for aou_req__platform__error_reaction: users of lm_control/lifecycle_client/Health Monitor shall check returned Results. Note: Lifecycle's own compliance as consumer of S-CORE functions should be confirmed by review.
   * - ``aou_req__platform__test_completion`` SW-platform test completion
     - Forwarded
     - 
     - Addressed to system integrator; platform-wide.
     - Covered at platform level.
   * - ``aou_req__platform__testing`` SW-platform testing
     - Forwarded
     - 
     - Integrator runs S-CORE tests; Lifecycle provides unit tests (\*_UT.cpp, Rust tests) and integration tests under tests/integration.
     - Covered at platform level.
   * - ``aou_req__platform__bazel_tooling`` Bazel tooling
     - Not applicable
     - 
     - AoU addressed to the supplier of an external SW element (OS/hypervisor/3rd-party lib). Lifecycle is an S-CORE module, not an external SW element supplier. Lifecycle itself is built with Bazel (MODULE.bazel).
     - 
   * - ``aou_req__platform__bug_fixing`` Bug fixing
     - Not applicable
     - 
     - AoU addressed to the supplier of an external SW element (OS/hypervisor/3rd-party lib). Lifecycle is an S-CORE module, not an external SW element supplier.
     - 
   * - ``aou_req__platform__bug_interface`` Bug interface
     - Not applicable
     - 
     - AoU addressed to the supplier of an external SW element (OS/hypervisor/3rd-party lib). Lifecycle is an S-CORE module, not an external SW element supplier. (Lifecycle bugs go via GitHub issues of eclipse-score/lifecycle anyway.)
     - 
   * - ``aou_req__platform__integration_assistance`` Integration assistance
     - Not applicable
     - 
     - AoU addressed to the supplier of an external SW element (OS/hypervisor/3rd-party lib). Lifecycle is an S-CORE module, not an external SW element supplier.
     - 
   * - ``aou_req__platform__os_integration_manual`` Integration manual
     - Not applicable
     - 
     - AoU addressed to the supplier of an external SW element (OS/hypervisor/3rd-party lib). Lifecycle is an S-CORE module, not an external SW element supplier.
     - 
   * - ``aou_req__platform__safety_anomaly`` Safety anomaly reporting
     - Not applicable
     - 
     - AoU addressed to the supplier of an external SW element (OS/hypervisor/3rd-party lib). Lifecycle is an S-CORE module, not an external SW element supplier.
     - 
   * - ``aou_req__platform__safety_aou`` Safety AoU
     - Not applicable
     - 
     - AoU addressed to the supplier of an external SW element (OS/hypervisor/3rd-party lib). Lifecycle is an S-CORE module, not an external SW element supplier. (Lifecycle provides its own AoUs as S-CORE module anyway.)
     - 
   * - ``aou_req__platform__safety_functions`` Safety functions
     - Not applicable
     - 
     - AoU addressed to the supplier of an external SW element (OS/hypervisor/3rd-party lib). Lifecycle is an S-CORE module, not an external SW element supplier.
     - 
   * - ``aou_req__platform__veh_time_e2e_integrity`` Vehicle time end-to-end integrity
     - Not applicable
     - 
     - Lifecycle uses only local monotonic time (CLOCK_MONOTONIC in LM, std::time::Instant in HM worker.rs:20); no vehicle/synchronized time consumed (no CLOCK_REALTIME/system_clock in LM src).
     - 
