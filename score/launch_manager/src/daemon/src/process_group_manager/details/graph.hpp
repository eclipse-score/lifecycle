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

#ifndef GRAPH_HPP_INCLUDED
#define GRAPH_HPP_INCLUDED

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "score/mw/launch_manager/common/concurrency/mpmc_concurrent_queue.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/osal/semaphore.hpp"
#include "score/mw/launch_manager/process_group_manager/details/component_event.hpp"
#include "score/mw/launch_manager/process_group_manager/details/component_of.hpp"
#include "score/mw/launch_manager/process_group_manager/details/component_task.hpp"
#include "score/mw/launch_manager/process_group_manager/details/dependency_graph.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_handling.hpp"
#include "score/mw/launch_manager/process_group_manager/details/process_info_node.hpp"
#include "score/mw/launch_manager/process_group_manager/details/run_target.hpp"
#include "score/mw/launch_manager/process_group_manager/details/transition.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include "score/mw/launch_manager/process_group_manager/irun_target_control.hpp"
#include "score/mw/lifecycle/details/lm_control_service.h"
#include <score/stop_token.hpp>

namespace score::mw::lifecycle::internal
{

using WorkerQueue =
    MPMCConcurrentQueue<std::optional<ComponentTask>, static_cast<std::size_t>(ProcessLimits::kMaxProcesses)>;

/// @brief Config members needed to build the graph
struct GraphConfig
{
    /// @brief Components that Run Targets may depend on
    std::vector<configuration::ComponentConfig> components_;
    /// @brief Run Targets that can be activated
    std::vector<configuration::RunTargetConfig> run_targets_;
    /// @brief Information about the Run Target transitioned to in the event of an error
    configuration::FallbackRunTargetConfig fallback_run_target_;
    /// @brief Name of the first Run Target to launch
    std::string initial_run_target_;
};

/// @brief GraphState - the graph/process group state.
/// @details Enumeration representing the state of the graph.
/// @note The allowed/disallowed states are managed by
/// the private method setState, which ensures valid transitions
/// between states. Invalid transitions are replaced by a valid new state.
/// The state transition logic is implemented in the setState method.
/// @verbatim
///    Old state    .  Requested State  .  New state
/// ----------------+-------------------+----------------
/// kSuccess        | kInTransition     | kInTransition
/// kSuccess        | kAborting         | kUndefinedState
/// kSuccess        | kUndefinedState   | kUndefinedState
/// ----------------+-------------------+----------------
/// kInTransition   | kSuccess          | kSuccess
/// kInTransition   | kAborting         | kAborting
/// kInTransition   | kUndefinedState   | kAborting
/// ----------------+-------------------+----------------
/// kAborting       | kSuccess          | kUndefinedState
/// kAborting       | kInTransition     | kAborting
/// kAborting       | kUndefinedState   | kUndefinedState
/// ----------------+-------------------+----------------
/// kUndefinedState | kSuccess          | kUndefinedState
/// kUndefinedState | kInTransition     | kInTransition
/// kUndefinedState | kAborting         | kUndefinedState
/// @endverbatim
enum class GraphState : std::uint_least8_t
{
    ///@brief Graph is not running and process group state is known
    kSuccess = 0U,

    ///@brief Graph is running, process group state is in transition
    kInTransition = 1U,

    ///@brief Graph is running but the transition has been aborted (error or cancellation because a new
    /// transition is pending); process group state is not known
    kAborting = 2U,

    ///@brief Graph is not running but process group state is not known
    kUndefinedState = 3U
};

/// @details Allowed transitions:
/// -------------------
/// kSuccess        -> kInTransition
/// kInTransition   -> kSuccess
/// kInTransition   -> kAborting
/// kInTransition   -> kUndefinedState
/// kAborting       -> kUndefinedState
/// kSuccess        -> kUndefinedState
/// kUndefinedState -> kInTransition
///
/// Disallowed transitions:             Replaced by
/// ------------------------------------------------
/// kSuccess        -> kAborting        kUndefinedState
/// kInTransition   -> kUndefinedState  kAborting
/// kAborting       -> kSuccess         kUndefinedState
/// kAborting       -> kInTransition    kAborting
/// kUndefinedState -> kSuccess         kUndefinedState
/// kUndefinedState -> kAborting        kUndefinedState
// clang-format off
static constexpr GraphState state_results[][static_cast<uint>(GraphState::kUndefinedState) + 1U] = {
    //from kSuccess                     kInTransition               kAborting                    kUndefinedState              to new_state
    {GraphState::kSuccess, GraphState::kSuccess, GraphState::kUndefinedState, GraphState::kUndefinedState},  // kSuccess
    {GraphState::kInTransition, GraphState::kInTransition, GraphState::kAborting, GraphState::kInTransition},  // kInTransition
    {GraphState::kUndefinedState, GraphState::kAborting, GraphState::kAborting, GraphState::kUndefinedState},  // kAborting
    {GraphState::kUndefinedState, GraphState::kAborting, GraphState::kUndefinedState, GraphState::kUndefinedState}  // kUndefinedState
};
// clang-format on

/// @brief Manages the processes and state transitions for a single process group.
///
/// Each Graph holds a set of ProcessInfoNode instances (one per process) arranged in a
/// dependency graph. During a state transition the Graph stops processes that are no longer
/// needed and starts the ones required for the new state, respecting dependency order. If
/// the transition completes without errors the graph enters kSuccess. Otherwise it enters
/// kUndefinedState.
class Graph final
{
  public:
    /// @brief All currently supported component implementations.
    using Component = std::variant<ProcessInfoNode, RunTarget>;

    static constexpr std::string_view off_state_name{"Off"};
    static constexpr std::string_view recovery_state_name{"fallback_run_target"};

    /// @brief Constructor to initialize a Graph object.
    /// @param max_num_nodes Maximum number of nodes this graph can hold.
    /// @param configuration Configuration containing Run Target and component information.
    /// @param job_queue Queue to push component jobs to for multithreaded processing.
    /// @param process_handling The interfaces used to start, stop and report on the OS processes.
    /// @param transition_result_receiver Object to notify when the initial transition is complete.
    Graph(
        uint32_t max_num_nodes,
        GraphConfig& configuration,
        std::shared_ptr<WorkerQueue> job_queue,
        ProcessHandling process_handling);

    /// @brief Destructor to clean up resources used by the Graph object.
    ~Graph();

    /// @brief Copy constructor (deleted).
    Graph(const Graph&) = delete;

    /// @brief Copy assignment operator (deleted).
    Graph& operator=(const Graph&) = delete;

    /// @brief Move constructor(deleted).
    Graph(Graph&&) noexcept = delete;

    /// @brief Move assignment operator(deleted).
    Graph& operator=(Graph&&) noexcept = delete;

    /// @brief Applies a ComponentEvent to this graph.
    /// @param event The event to process.
    void handleComponentEvent(const ComponentEvent& event);

    /// @brief Cancel the current transition because a new state has been requested.
    /// Sets the graph state to kAborting.
    /// If no jobs are in progress, transitions immediately to kUndefinedState.
    void cancel();

    /// @brief Begin transitioning this process group to the given state.
    /// @return False if pg_state is not a recognized Run Target in this graph's configuration; the
    /// transition is not started in that case. True otherwise.
    /// @param pg_state The target process group state.
    bool startTransition(IdentifierHash pg_state);

    /// @return True if pg_state is a Run Target known to this graph's configuration.
    /// @param pg_state The process group state to check.
    bool isValidRunTarget(IdentifierHash pg_state);

    /// @brief Begin the initial machine group startup transition.
    /// Behaves like startTransition but also reports the initial state transition result
    /// to the ProcessGroupManager on failure.
    /// @param pg_state The initial machine group startup state.
    void startInitialTransition(IdentifierHash pg_state);

    /// @brief Begin transitioning this process group to the "Off" state.
    /// Stops all processes in the group even if no explicit "Off" state is configured.
    /// @return True if the transition was started. False if the graph could not enter kInTransition.
    bool startTransitionToOffState();

    /// @return True if the graph is currently transitioning to the Off state.
    bool isTransitioningToOff() const;

    /// @return The current graph state.
    GraphState getState() const;

    /// @param process_index Index of the process node to retrieve.
    /// @return The ProcessInfoNode at the given index, or nullptr if out of bounds or if the node
    /// at that index is a RunTarget rather than a ProcessInfoNode.
    ProcessInfoNode* getProcessInfoNode(IdentifierHash process_index);

    /// @return The currently requested Run Target.
    /// @note Only meaningful when getState() returns GraphState::kSuccess.
    IdentifierHash getRequestedRunTarget();

    /// @brief Update the details for the cancel message to match the current state.
    void updateCancelMessage();

    /// @return The error code set by the last process that caused an unexpected termination.
    uint32_t getLastExecutionError();

    /// @brief Stores an error code representing the last execution failure.
    /// @param code The error code to store.
    void setLastExecutionError(uint32_t code);

    /// @brief Replaces the pending state with new_state and returns the previous pending state.
    /// @param new_state The new pending state to set.
    /// @return The previous pending state.
    IdentifierHash setPendingState(IdentifierHash new_state);

    /// @return The pending state, or an empty hash if no state is pending.
    IdentifierHash getPendingState();

    /// @brief A utility function that converts codes to strings for logging purposes
    /// @param state The state to convert
    /// @return A string representing the state
    static std::string_view toString(GraphState state);

    /// @brief Records the current time as the start of a state transition request.
    void setRequestStartTime();

    /// @return The timestamp recorded at the start of the current state transition request.
    std::chrono::time_point<std::chrono::steady_clock> getRequestStartTime();

    /// @brief For forced shutdown, kill all leftover processes
    void forceKillProcesses();

    /// @brief Returns the configured transition timeout for the Off state
    /// @details This is the timeout configured for the RunTarget named "Off" in the configuration, or a default value
    /// if not configured.
    /// @return The timeout in milliseconds, or zero if there is no configured timeout.
    std::chrono::milliseconds getOffStateTransitionTimeout() const;

    /// @brief Register a callback to be fired when the active Run Target changes.
    void registerActiveRunTargetCallback(ActivationCallbackT callback) noexcept;

  private:
    /// @brief Reports that a node has finished executing, enqueuing successors or updating the graph state if a
    /// transition has finished.
    void nodeExecuted(IdentifierHash node, score::cpp::expected_blank<IComponent::ComponentError> error);

    /// @brief Sets the current state of the graph.
    /// @param new_state The new state to set for the graph.
    /// @returns False if the requested state was not set
    bool setState(GraphState new_state);

    /// @brief Pushes the given task onto the worker queue while the graph is in transition.
    /// Retries on timeout.
    /// @param task The task to enqueue.
    void tryQueueNode(ComponentTask task);

    /// @brief Every node that is ready to execute is either executed in place (RunTarget) or queued for execution
    /// (ProcessInfoNode).
    void queueReadyNodes();

    /// @brief Executes a RunTarget's activation/deactivation in place
    /// @details Since a RunTarget is a virtual node with no work to do
    /// and reports its completion to the current transition immediately.
    void updateRunTargetInPlace(RunTarget& run_target, ComponentTaskType task_type);

    /// @brief Common tail of a transition that finished without error: moves the graph to
    /// kSuccess, posts kSetStateSuccess, and reports initial-state-transition success if this
    /// was the initial transition.
    void finalizeTransitionSuccess();

    /// @brief Finalizes an aborted transition (error or cancellation) after the last in-flight job
    /// completes. Moves the graph state to kUndefinedState and posts the appropriate event.
    void handleNonTransitionExecution();

    /// @brief Number of jobs that have been queued but are not yet executed
    int32_t jobs_in_progress_{0};

    /// @brief Nodes for all unique processes in this process group, plus a virtual RunTarget node
    /// per configured ProcessGroupState.
    DependencyGraph<IdentifierHash, Component> nodes_;

    /// @brief Builder for creating the transition object for the current state transition.
    TransitionBuilder<IdentifierHash, Component> transition_builder_;

    /// @brief The currently active transition or nullptr before the first one starts.
    Transition<IdentifierHash, Component>* current_transition_{nullptr};

    /// @brief Current state of the graph.
    GraphState state_{GraphState::kSuccess};

    /// @brief the requested Run Target.
    IdentifierHash requested_state_{};

    /// @brief Mutex protecting concurrent access to requested_state_.
    mutable std::mutex requested_state_mutex_{};

    /// @brief Config pointer to set up graph nodes
    GraphConfig& configuration_;

    /// @brief Queue to push component tasks to
    std::shared_ptr<WorkerQueue> job_queue_;

    /// @brief The interfaces passed to the process nodes to control their OS processes
    ProcessHandling process_handling_;

    /// @brief Set the true if this is the initial state transition
    bool is_initial_state_transition_{false};

    /// @brief The pending state transition, if any
    IdentifierHash pending_state_{""};

    /// @brief Constant for Off state.
    const IdentifierHash off_state_{"Off"};

    /// @brief Stores the timestamp based on the system clock when starting a request
    std::chrono::time_point<std::chrono::steady_clock> request_start_time_{};

    /// @brief Stop token generator for transitions.
    score::cpp::stop_source stop_source_;

    /// @brief Transition timeout for Off state
    std::chrono::milliseconds off_state_transition_timeout_{0};

    std::optional<ActivationCallbackT> active_run_target_callback_;
};

}  // namespace score::mw::lifecycle::internal

#endif  /// GRAPH_HPP_INCLUDED
