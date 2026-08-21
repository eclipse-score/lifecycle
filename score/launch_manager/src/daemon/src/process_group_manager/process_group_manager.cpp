/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>

#include "score/mw/com/types.h"
#include "score/mw/launch_manager/common/log.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_monitor.hpp"
#include "score/mw/launch_manager/process_group_manager/ialive_monitor_thread.hpp"
#include "score/mw/launch_manager/process_group_manager/process_group_manager.hpp"
#include "score/mw/lifecycle/details/lm_control_service.h"

namespace score::mw::lifecycle::internal
{

static std::atomic_bool em_cancelled{false};

static void my_signal_handler(int)
{
    em_cancelled.store(true);
}

void ProcessGroupManager::cancel()
{
    my_signal_handler(SIGTERM);
}

ProcessGroupManager::ProcessGroupManager(
    std::unique_ptr<IAliveMonitorThread> alive_monitor_thread,
    std::shared_ptr<IRecoveryClient> recovery_client,
    std::unique_ptr<score::mw::lifecycle::ISupervisionControlNotifier> supervision_control_notifier,
    std::unique_ptr<score::mw::lifecycle::internal::watchdog::IWatchdogIf> watchdog)
    : configuration_(),
      process_interface_(),
      process_map_(nullptr),
      worker_threads_(nullptr),
      worker_jobs_(nullptr),
      num_process_groups_(0U),
      process_groups_(),
      supervision_control_notifier_(std::move(supervision_control_notifier)),
      alive_monitor_thread_(std::move(alive_monitor_thread)),
      recovery_client_(recovery_client),
      watchdog_(std::move(watchdog))
{
}

bool ProcessGroupManager::initialize(const Config& config)
{
    // setup signal handler
    em_cancelled.store(false);
    // RULECHECKER_comment(1, 1, check_union_object, "Union type defined in external library is used.", true)
    struct sigaction action;

    action.sa_handler = my_signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGALRM, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGIO, &action, NULL);
    sigaction(SIGPROF, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGUSR1, &action, NULL);
    sigaction(SIGUSR2, &action, NULL);
    sigaction(SIGVTALRM, &action, NULL);

    if (!configuration_.initialize(config))
    {
        LM_LOG_ERROR() << "Failed to initialize config";
        return false;
    }

    const auto* pg_list = configuration_.getListOfProcessGroups().value_or(nullptr);
    if (!pg_list || pg_list->empty())
    {
        LM_LOG_ERROR() << "Failed to get pg list";
        return false;
    }

    std::uint32_t total_processes = 0;
    for (const auto pg_name : *pg_list)
    {
        total_processes += configuration_.getNumberOfOsProcesses(pg_name).value_or(0);
    }

    if (total_processes > static_cast<uint32_t>(ProcessLimits::kMaxProcesses))
    {
        LM_LOG_ERROR() << "Too many processes";
        return false;
    }

    createProcessComponentsObjects(total_processes);

    if (!initializeProcessGroups())
    {
        return false;
    }

    LM_LOG_DEBUG() << "Process Group initialization done";
    initializeGraphNodes();
    if (!alive_monitor_thread_->start())
    {
        LM_LOG_ERROR() << "Alive monitor thread failed to start";
        return false;
    }

    const auto watchdog_config = config.watchdog();

    // Watchdog config may not be available if no watchdog is configured
    if (watchdog_config.has_value())
    {
        if (!watchdog_->init(watchdog_config.value(), score::mw::lifecycle::internal::kMainLoopCycleTimeNs))
        {
            LM_LOG_ERROR() << "Watchdog initialization failed";
            return false;
        }
        if (!watchdog_->enable())
        {
            LM_LOG_ERROR() << "Watchdog enable failed";
            return false;
        }
    }

    return true;
}

void ProcessGroupManager::deinitialize()
{
    // ucm_polling_thread_.stopPolling();
    watchdog_->disable();
    if (event_queue_)
    {
        event_queue_->stop();
    }
    os_handler_.reset();
    process_monitor_.reset();
    alive_monitor_thread_->stop();
    configuration_.deinitialize();
    process_groups_.clear();

    worker_threads_.reset();
    worker_jobs_.reset();
    process_map_.reset();
}

bool ProcessGroupManager::initializeProcessGroups()
{
    bool success = false;

    auto pg_list = configuration_.getListOfProcessGroups().value_or(nullptr);

    if (pg_list && !pg_list->empty())
    {
        num_process_groups_ = static_cast<uint32_t>(pg_list->size() & 0xFFFFFFFFUL);
        LM_LOG_DEBUG() << num_process_groups_ << "process group(s)";

        success = true;

        for (const auto& pg_name : *pg_list)
        {
            uint32_t num_processes = configuration_.getNumberOfOsProcesses(pg_name).value_or(0);
            const auto* states = configuration_.getListOfProcessGroupStates(pg_name).value_or(nullptr);
            const uint32_t num_run_targets = states ? static_cast<uint32_t>(states->size()) : 0U;

            process_groups_.push_back(std::make_shared<Graph>(
                num_processes + num_run_targets,
                &configuration_,
                worker_jobs_,
                &process_interface_,
                process_map_,
                *supervision_control_notifier_.get()));
        }
    }
    else
    {
        LM_LOG_ERROR() << "No process groups";
    }

    if (success)
    {
        LM_LOG_DEBUG() << "Process groups initialized successfully";
    }
    else
    {
        LM_LOG_ERROR() << "Failed to initialize process groups";
    }

    return success;
}

void ProcessGroupManager::createProcessComponentsObjects(std::size_t total_processes)
{
    LM_LOG_DEBUG() << "Creating component event queue...";
    event_queue_ = std::make_unique<ComponentEventQueue>(total_processes);

    if (recovery_client_)
    {
        recovery_client_->setRecoveryRequestCallback([this](const IdentifierHash& process_identifier) {
            static_cast<void>(event_queue_->push(SupervisionFailure{process_identifier}));
        });
    }

    LM_LOG_DEBUG() << "Creating process monitor...";
    process_monitor_ = std::make_unique<ProcessMonitor>(*event_queue_);

    LM_LOG_DEBUG() << "Creating Safe Process Map with" << total_processes << "entries";
    process_map_ = std::make_shared<SafeProcessMap>(total_processes, *process_monitor_);

    LM_LOG_DEBUG() << "Creating OS handler...";
    os_handler_ = std::make_unique<OsHandler>(*process_map_);

    LM_LOG_DEBUG() << "Creating job queue with capacity" << static_cast<std::size_t>(ProcessLimits::kMaxProcesses);
    worker_jobs_ = std::make_shared<WorkerQueue>();

    LM_LOG_DEBUG() << "Creating worker threads...";
    worker_threads_ = std::make_unique<WorkerThread<ComponentTask>>(
        worker_jobs_, static_cast<uint32_t>(ProcessLimits::kNumWorkerThreads), *process_monitor_);
}

void ProcessGroupManager::initializeGraphNodes()
{
    auto pg_list = configuration_.getListOfProcessGroups().value_or(nullptr);

    for (size_t idx = 0U; idx < process_groups_.size(); ++idx)
    {
        process_groups_[idx]->initProcessGroupNodes(
            pg_list->at(idx),
            configuration_.getNumberOfOsProcesses(pg_list->at(idx)).value_or(0U),
            static_cast<uint32_t>(idx & 0xFFFFFFFFUL));
    }

    LM_LOG_DEBUG() << "Graphs initialized";
}

void ProcessGroupManager::offerService()
{
    const auto instance_specifier =
        score::mw::com::InstanceSpecifier::Create(std::string{"LaunchManager/StateManager/Instance"});
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(instance_specifier.has_value());

    auto instance_result = LmControlSkeleton::Create(instance_specifier.value());
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(instance_result.has_value());
    auto instance = std::move(instance_result).value();

    const auto register_result = instance.activate_run_target.RegisterHandler(
        [this](ActivateRunTargetResponse& response, const ActivateRunTargetRequest& request) {
            SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
                request.mode == ActivationMode::kForced, "Only ActivationMode::kForced is implemented");

            IdentifierHash new_state = IdentifierHash(request.run_target_name.as_string_view());

            Graph& graph = *process_groups_.front();
            graph.setPendingState(new_state);

            response = ActivateRunTargetResponse{status : RequestStatus::kRejected};
        });
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(register_result.has_value());

    const auto offer_result = instance.OfferService();
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(offer_result.has_value());
}

bool ProcessGroupManager::run()
{
    // RULECHECKER_comment(1, 4, check_c_style_cast, "This is the definition provided by the OS and does a C-style
    // cast.", true)
    LM_LOG_DEBUG() << "clock() at run():"
                   // coverity[cert_err33_c_violation:INTENTIONAL] Does not matter if clock() gives a weird value in
                   // debug messages.
                   << (static_cast<double>(clock()) / (static_cast<double>(CLOCKS_PER_SEC) / 1000.0)) << "ms";

    bool result = startInitialTransition();
    bool overflow_logged = false;

    this->offerService();

    if (result)
        while (!em_cancelled.load())
        {
            // Wait for something to happen...
            // The wait is kept below the minimum watchdog timeout so that the wait plus per-iteration
            // processing stays within budget for servicing the watchdog each cycle.

            // Wait for a graph-relevant event (activation/deactivation completion or
            // unexpected termination). All Graph state mutations happen here, on the main thread.
            if (event_queue_->waitForEvents(
                    std::chrono::milliseconds(score::mw::lifecycle::internal::kMainLoopCycleTimeMs)))
            {
                processComponentEvents();
            }

            if (event_queue_->getOverflow() && !overflow_logged)
            {
                LM_LOG_FATAL() << "ComponentEventQueue overflow - one or more events were lost";
                overflow_logged = true;
                watchdog_->fireWatchdogReaction();
            }

            for (auto pg : process_groups_)
            {
                processGroupHandler(*pg);
            }

            watchdog_->serviceWatchdog();
        }

    allProcessGroupsOff();

    return result;
}

void ProcessGroupManager::processComponentEvents()
{
    // Single-graph assumption: PGM creates one ProcessMonitor bound to the first (only) graph, so
    // every event always applies to it. Multi-graph routing is deferred to a future revision.
    Graph& graph = *process_groups_.front();

    while (auto event = event_queue_->getNextEvent())
    {
        if (const auto* supervision_failure = std::get_if<SupervisionFailure>(&*event))
        {
            handleRecoveryRequest(supervision_failure->process_identifier);
        }
        else
        {
            graph.handleComponentEvent(*event);
        }
    }
}

bool ProcessGroupManager::startInitialTransition()
{
    bool result = false;
    LM_LOG_DEBUG() << "=============STARTING MAINPG STARTUP STATE============";

    // Initial transition of machine process group
    const ProcessGroupStateID* pg_startup_id = configuration_.getMainPGStartupState().value_or(nullptr);

    if (pg_startup_id)
    {
        machine_process_group_ = getProcessGroup(pg_startup_id->pg_name_);

        if (machine_process_group_)
        {
            machine_process_group_->startInitialTransition(pg_startup_id->pg_state_name_);
            result = true;
        }
    }
    else
    {
        LM_LOG_ERROR() << "No startup state, exiting from process group manager";
    }
    return result;
}

void ProcessGroupManager::allProcessGroupsOff()
{
    // Wait for process group states to change while actively draining shutdown events.
    // SupervisionFailure is intentionally ignored here so recovery transitions do not
    // fight the forced transition to Off.
    auto waitForStateCompletion = [this](GraphState state_to_be_completed, int32_t max_wait_ms) -> bool {
        constexpr int32_t kSleepIntervalMs = 10;

        Graph& graph = *process_groups_.front();
        auto has_state = [&graph, state_to_be_completed]() {
            return graph.getState() == state_to_be_completed;
        };

        int32_t remaining_ms = max_wait_ms;
        while (has_state() && (remaining_ms > 0))
        {
            static_cast<void>(event_queue_->waitForEvents(std::chrono::milliseconds(kSleepIntervalMs)));
            while (auto event = event_queue_->getNextEvent())
            {
                if (std::holds_alternative<SupervisionFailure>(*event))
                {
                    continue;
                }
                graph.handleComponentEvent(*event);
            }

            remaining_ms -= kSleepIntervalMs;
        }

        return !has_state();
    };

    Graph& graph = *process_groups_.front();
    // First, check if we're already transitioning to Off - if so, no need to cancel
    if (!graph.isTransitioningToOff())
    {
        // Cancel any pending transitions that are not going to Off
        LM_LOG_DEBUG() << "Cancel all process group transitions";
        graph.cancel();

        // Wait for cancellation to complete
        LM_LOG_DEBUG() << "Wait for process group cancellations";
        if (!waitForStateCompletion(GraphState::kCancelled, 2000))
        {
            LM_LOG_ERROR() << "NOTE: Cancellation timed out";
        }

        // Start transitioning all process groups to the "Off" state
        LM_LOG_DEBUG() << "Start transitioning process groups to Off state";
        (void)graph.startTransitionToOffState();
    }
    else
    {
        LM_LOG_DEBUG() << "Already transitioning to Off state, skipping cancellation";
    }

    LM_LOG_DEBUG() << "Wait for all process groups to complete the transition";
    if (!waitForStateCompletion(GraphState::kInTransition, 1000))
    {
        LM_LOG_ERROR() << "NOTE: Transition to Off state timed out";
        worker_threads_->stop();

        for (auto& pg : process_groups_)
        {
            pg->forceKillProcesses();
        }
    }
}

void ProcessGroupManager::handleRecoveryRequest(const IdentifierHash& process_identifier)
{
    auto pg = getProcessGroupByProcessId(process_identifier);

    if (nullptr == pg)
    {
        LM_LOG_ERROR() << "handleRecoveryRequest: Unknown process " << process_identifier;
        return;
    }

    const IdentifierHash old_state = pg->getProcessGroupState();
    const IdentifierHash recovery_state = configuration_.getNameOfRecoveryState(pg->getProcessGroupName());
    const GraphState graph_state = pg->getState();

    LM_LOG_DEBUG() << "handleRecoveryRequest: Processing recovery request for PG " << process_identifier << " to state "
                   << recovery_state;

    if (GraphState::kInTransition == graph_state)
    {
        if (old_state != recovery_state)
        {
            // Cancel current transition and start new one
            (void)pg->setPendingState(recovery_state);
            pg->setRequestStartTime();
            pg->cancel();
        }
        else
        {
            // Already in transition to the requested state
            LM_LOG_DEBUG() << "handleRecoveryRequest: Already transitioning to same state";
        }
    }
    else if (GraphState::kSuccess == graph_state && old_state == recovery_state)
    {
        // Already in the requested state
        LM_LOG_DEBUG() << "handleRecoveryRequest: Already in requested state";
    }
    else
    {
        // Start new state transition
        (void)pg->setPendingState(recovery_state);
        pg->setRequestStartTime();
    }
}

void ProcessGroupManager::processGroupHandler(Graph& pg)
{
    // check to see if there is a state change request to process
    // If current pg not in transition and there is a pending request state
    // start the transition, produce immediate response if that fails.
    GraphState graph_state = pg.getState();

    if (GraphState::kSuccess == graph_state || GraphState::kUndefinedState == graph_state)
    {
        ProcessGroupStateID pgs;
        pgs.pg_state_name_ = pg.setPendingState(IdentifierHash(""));

        if ((pgs.pg_state_name_ != IdentifierHash("")) &&
            ((pgs.pg_state_name_ != pg.getProcessGroupState()) || (GraphState::kUndefinedState == graph_state)))
        {
            pgs.pg_name_ = pg.getProcessGroupName();
            LM_LOG_DEBUG() << "Start transition to" << pgs.pg_state_name_ << "for PG" << pgs.pg_name_;

            pg.startTransition(pgs.pg_state_name_);
        }

        if (GraphState::kUndefinedState == pg.getState())
        {
            // at the moment graph is not running...
            // i.e. it is not in kInTransition, kAborting or kCancelled state
            //
            // in short, graph is in an error state (kUndefinedState)
            // and there is no valid request from outside, to change this situation...
            //
            // we will try to perform recovery action

            ProcessGroupStateID recovery_state;
            recovery_state.pg_name_ = pg.getProcessGroupName();
            recovery_state.pg_state_name_ = configuration_.getNameOfRecoveryState(recovery_state.pg_name_);

            LM_LOG_WARN() << "Problem discovered in PG" << recovery_state.pg_name_ << "Activating Recovery state.";

            // no point checking errors here...
            // nobody requested this transition, so there is nowhere to communicate an error
            // if we failed and there is no external request, we will try again next time
            pg.setRequestStartTime();
            pg.startTransition(recovery_state.pg_state_name_);
        }
    }
}

ProcessInfoNode* ProcessGroupManager::getProcessInfoNode(uint32_t pg_index, uint32_t process_index)
{
    if (pg_index < process_groups_.size())
    {
        return process_groups_[pg_index]->getProcessInfoNode(process_index);
    }

    return nullptr;
}

std::shared_ptr<Graph> ProcessGroupManager::getProcessGroupByProcessId(const IdentifierHash& process_id)
{
    for (auto& pg : process_groups_)
    {
        const IdentifierHash pg_name = pg->getProcessGroupName();
        const uint32_t count = configuration_.getNumberOfOsProcesses(pg_name).value_or(0U);
        for (uint32_t idx = 0U; idx < count; ++idx)
        {
            const auto* proc = configuration_.getOsProcessConfiguration(pg_name, idx).value_or(nullptr);
            if (proc != nullptr && proc->process_id_ == process_id)
            {
                return pg;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<Graph> ProcessGroupManager::getProcessGroup(IdentifierHash pg_name)
{
    /* we could use a map, we could use std::find_if
       however, there's not many process groups so gain of using a map
       is small, and it seems simpler just to write a simple loop.
     */
    std::shared_ptr<Graph> result = nullptr;

    for (auto pg : process_groups_)
    {
        if (pg->getProcessGroupName() == pg_name)
        {
            result = pg;
            break;
        }
    }

    return result;
}

osal::IProcess* ProcessGroupManager::getProcessInterface()
{
    return &process_interface_;
}

ConfigurationType* ProcessGroupManager::getConfiguration()
{
    return &configuration_;
}

std::shared_ptr<SafeProcessMap> ProcessGroupManager::getProcessMap()
{
    return process_map_;
}

std::shared_ptr<ProcessGroupManager::WorkerQueue> ProcessGroupManager::getWorkerJobs()
{
    return worker_jobs_;
}

}  // namespace score::mw::lifecycle::internal
