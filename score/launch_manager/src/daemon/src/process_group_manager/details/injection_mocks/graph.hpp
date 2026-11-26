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

#include <chrono>
#include <memory>
#include <string_view>
#include <variant>

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
#include "score/mw/lifecycle/run_target_activation_source.hpp"
#include <score/stop_token.hpp>

#include <gmock/gmock.h>

namespace score::mw::lifecycle::internal
{

using WorkerQueue =
    MPMCConcurrentQueue<std::optional<ComponentTask>, static_cast<std::size_t>(ProcessLimits::kMaxProcesses)>;

enum class GraphState : std::uint_least8_t
{
    kSuccess = 0U,
    kInTransition = 1U,
    kAborting = 2U,
    kUndefinedState = 3U
};

static constexpr GraphState state_results[][static_cast<uint>(GraphState::kUndefinedState) + 1U] = {
    // from kSuccess                     kInTransition               kAborting                    kUndefinedState to
    // new_state
    {GraphState::kSuccess, GraphState::kSuccess, GraphState::kUndefinedState, GraphState::kUndefinedState},  // kSuccess
    {GraphState::kInTransition,
     GraphState::kInTransition,
     GraphState::kAborting,
     GraphState::kInTransition},  // kInTransition
    {GraphState::kUndefinedState,
     GraphState::kAborting,
     GraphState::kAborting,
     GraphState::kUndefinedState},  // kAborting
    {GraphState::kUndefinedState,
     GraphState::kAborting,
     GraphState::kUndefinedState,
     GraphState::kUndefinedState}  // kUndefinedState
};

struct PendingTransition
{
    IdentifierHash run_target{""};
    RunTargetActivationSource source{RunTargetActivationSource::kStateManagerRequest};
};

class MockGraph
{
  public:
    MOCK_METHOD(
        void,
        ctor,
        (uint32_t max_num_nodes,
         configuration::GraphConfig& configuration,
         std::shared_ptr<WorkerQueue> job_queue,
         ProcessHandling process_handling),
        ());
    MOCK_METHOD(void, dtor, (), ());
    MOCK_METHOD(void, handleComponentEvent, (const ComponentEvent& event), ());
    MOCK_METHOD(void, unblockComponentEvent, (const ComponentEvent& event, const ExecErrc reason), ());
    MOCK_METHOD(void, cancel, (), ());
    MOCK_METHOD(bool, startTransition, (IdentifierHash pg_state, RunTargetActivationSource source), ());
    MOCK_METHOD(bool, isValidRunTarget, (IdentifierHash pg_state), ());
    MOCK_METHOD(void, startInitialTransition, (IdentifierHash pg_state), ());
    MOCK_METHOD(bool, startTransitionToOffState, ());
    MOCK_METHOD(bool, isTransitioningToOff, (), (const));
    MOCK_METHOD(GraphState, getState, (), (const));
    MOCK_METHOD(IdentifierHash, getRequestedRunTarget, ());
    MOCK_METHOD(IdentifierHash, setPendingState, (IdentifierHash new_state, RunTargetActivationSource source), ());
    MOCK_METHOD(PendingTransition, getPendingTransition, (), (const));
    MOCK_METHOD(std::string_view, toString, (GraphState state), ());
    MOCK_METHOD(void, setRequestStartTime, (), ());
    MOCK_METHOD(std::chrono::time_point<std::chrono::steady_clock>, getRequestStartTime, (), ());
    MOCK_METHOD(void, forceKillProcesses, (), ());
    MOCK_METHOD(std::chrono::milliseconds, getOffStateTransitionTimeout, (), (const));
    MOCK_METHOD(void, registerActiveRunTargetCallback, (ActivationCallbackT callback), (noexcept));
};

/// @brief @see tests/utils/mocks/README.md#Header%20replacement for explanation and usage
extern std::unique_ptr<MockGraph> g_mock_graph;

class Graph
{
  public:
    using Component = std::variant<ProcessInfoNode, RunTarget>;

    static constexpr std::string_view off_state_name{"Off"};
    static constexpr std::string_view recovery_state_name{"fallback_run_target"};

    Graph(
        uint32_t max_num_nodes,
        configuration::GraphConfig& configuration,
        std::shared_ptr<WorkerQueue> job_queue,
        ProcessHandling process_handling)
    {
        if (g_mock_graph)
        {
            g_mock_graph->ctor(max_num_nodes, configuration, job_queue, process_handling);
        }
    }

    ~Graph()
    {
        g_mock_graph->dtor();
    }

    void handleComponentEvent(const ComponentEvent& event)
    {
        g_mock_graph->handleComponentEvent(event);
    }

    void unblockComponentEvent(const ComponentEvent& event, const ExecErrc reason)
    {
        g_mock_graph->unblockComponentEvent(event, reason);
    }

    void cancel()
    {
        g_mock_graph->cancel();
    }

    bool startTransition(
        IdentifierHash pg_state,
        RunTargetActivationSource source = RunTargetActivationSource::kStateManagerRequest)
    {
        return g_mock_graph->startTransition(pg_state, source);
    }

    bool isValidRunTarget(IdentifierHash pg_state)
    {
        return g_mock_graph->isValidRunTarget(pg_state);
    }

    void startInitialTransition(IdentifierHash pg_state)
    {
        g_mock_graph->startInitialTransition(pg_state);
    }

    bool startTransitionToOffState()
    {
        return g_mock_graph->startTransitionToOffState();
    }

    bool isTransitioningToOff() const
    {
        return g_mock_graph->isTransitioningToOff();
    }

    GraphState getState() const
    {
        return g_mock_graph->getState();
    }

    IdentifierHash getRequestedRunTarget()
    {
        return g_mock_graph->getRequestedRunTarget();
    }

    IdentifierHash setPendingState(
        IdentifierHash new_state,
        RunTargetActivationSource source = RunTargetActivationSource::kStateManagerRequest)
    {
        return g_mock_graph->setPendingState(new_state, source);
    }

    PendingTransition getPendingTransition() const
    {
        return g_mock_graph->getPendingTransition();
    }

    static std::string_view toString(GraphState state)
    {
        return g_mock_graph->toString(state);
    }

    void setRequestStartTime()
    {
        g_mock_graph->setRequestStartTime();
    }

    std::chrono::time_point<std::chrono::steady_clock> getRequestStartTime()
    {
        return g_mock_graph->getRequestStartTime();
    }

    void forceKillProcesses()
    {
        g_mock_graph->forceKillProcesses();
    }

    std::chrono::milliseconds getOffStateTransitionTimeout() const
    {
        return g_mock_graph->getOffStateTransitionTimeout();
    }

    void registerActiveRunTargetCallback(ActivationCallbackT callback) noexcept
    {
        g_mock_graph->registerActiveRunTargetCallback(callback);
    }
};

}  // namespace score::mw::lifecycle::internal

#endif  /// GRAPH_HPP_INCLUDED
