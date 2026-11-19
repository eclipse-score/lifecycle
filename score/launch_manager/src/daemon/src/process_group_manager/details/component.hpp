/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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

#ifndef SCORE_LCM_COMPONENT_HPP_INCLUDED
#define SCORE_LCM_COMPONENT_HPP_INCLUDED

#include "score/mw/launch_manager/alive_monitor/ialive_supervision_handle.hpp"
#include "score/mw/launch_manager/alive_monitor/isupervision_factory.hpp"
#include "score/mw/launch_manager/configuration/component_config.hpp"
#include "score/mw/launch_manager/process_group_manager/details/force_stop_action/iforce_stop_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/icomponent.hpp"
#include "score/mw/launch_manager/process_group_manager/details/ready_condition/iready_condition.hpp"
#include "score/mw/launch_manager/process_group_manager/details/start_action/istart_action.hpp"
#include "score/mw/launch_manager/process_group_manager/details/stop_action/istop_action.hpp"
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <variant>

namespace score::mw::lifecycle::internal
{

/// @brief Manages and tracks the state of a resource.
/// @details activate() and deactivate() never run at the same time. The graph guarantees this, and the
///          component asserts it. Terminations of the resource, supervision failures and forced stops
///          can be reported concurrently to both.
///          Every failure ends in the inactive state, so the component can always be activated again.
class Component final : public IComponent
{
  public:
    /// @brief Creates a new component from its configuration and the given actions.
    /// @param start_action How to start the resource.
    /// @param stop_action How to gracefully stop the resource.
    /// @param force_stop_action How to forcefully stop the resource.
    /// @param ready_condition How to decide when the resource has finished its startup.
    /// @param supervision_factory Factory used to construct alive supervision.
    /// @param config Configuration of the component.
    Component(
        std::unique_ptr<const IStartAction>&& start_action,
        std::unique_ptr<const IStopAction>&& stop_action,
        std::unique_ptr<const IForceStopAction>&& force_stop_action,
        std::unique_ptr<const IReadyCondition>&& ready_condition,
        ISupervisionFactory& supervision_factory,
        std::shared_ptr<configuration::ComponentConfig>&& config);

    /// @brief Start the resource and set the component to active.
    /// @details If an attempt fails, the resource is stopped and its termination is waited for, so that
    ///          nothing is left running. A failed attempt is retried up to `activation_retry_attempts`
    ///          times, each with the full ready timeout. Retrying ends early if the stop token was stopped
    ///          meanwhile, or if the resource could not be cleaned up, since another attempt could then
    ///          start a second instance of it. The component is inactive after a failed activation.
    /// @pre deactivate() is not running.
    /// @param stop_token Token which can be used to interrupt the wait for the ready condition.
    ///                   It does not interrupt stopping the resource after a failed activation.
    /// @return Whether the component activation succeeded, or the result of the last failed attempt.
    RequestResult activate(cpp::stop_token stop_token) override;

    /// @brief Stop the resource and set the component to inactive.
    /// @details Waits for the termination of the resource. If it does not terminate within the shutdown
    ///          timeout, it is stopped forcefully. The component is inactive afterwards in any case.
    /// @pre activate() is not running.
    /// @param stop_token Ignored: stopping the resource cannot be interrupted.
    /// @return kSuccess if the resource is gone, kFailure if it could not be cleaned up.
    RequestResult deactivate(cpp::stop_token stop_token) override;

    /// @brief Notify the component that a POSIX process has terminated.
    /// @details A termination during startup is reported to the ready condition and always ignored here;
    ///          activate() reports the outcome of the activation instead.
    /// @param status Exit code of the process.
    /// @return Whether the termination is ignored or means the component failed.
    TerminationResult tryHandleTermination(int32_t status) override;

    /// @brief Notify the component that its alive supervision has failed.
    /// @details Stops the resource forcefully if the component is ready and the failure belongs to the
    ///          current supervision. The termination is then reported as unexpected.
    /// @param failure_timestamp Monotonic timestamp of the supervision failure.
    void handleSupervisionFailure(std::chrono::nanoseconds failure_timestamp) override;

    /// @brief Forcefully stop the resource, if it is running.
    /// @details Does not wait for the termination.
    void forceStop();

    /// @brief Return the name of the component.
    /// @return The name of the component.
    [[nodiscard]] IdentifierHash getIdentifier() const override;

    /// @brief Return whether the component is active.
    /// @return Whether the component is active.
    [[nodiscard]] bool active() const override;

  private:
    /// @brief How to start the resource.
    const std::unique_ptr<const IStartAction> start_action_;

    /// @brief How to gracefully stop the resource.
    const std::unique_ptr<const IStopAction> stop_action_;

    /// @brief How to forcefully stop the resource.
    const std::unique_ptr<const IForceStopAction> force_stop_action_;

    /// @brief How to decide when the resource has finished its startup.
    const std::unique_ptr<const IReadyCondition> ready_condition_;

    /// @brief Configuration of the component.
    const std::shared_ptr<configuration::ComponentConfig> config_;

    /// @brief Alive supervision of the resource, or nullptr if it is not supervised.
    const std::unique_ptr<IAliveSupervisionHandle> supervision_handle_;

    /// @brief No resource is running.
    struct TerminatedState final
    {
    };

    /// @brief The resource was started, activate() waits for its ready condition.
    struct StartingState final
    {
        Handle handle_;

        /// @brief Whether the resource terminated in a way which fails the activation.
        bool terminated_unexpectedly_{false};
    };

    /// @brief The resource has finished its startup.
    struct ReadyState final
    {
        Handle handle_;

        /// @brief Monotonic time at which supervision was activated, 0 if unsupervised.
        std::chrono::nanoseconds supervision_start_;
    };

    /// @brief The resource was asked to stop, stopAndWait() waits for its termination.
    struct TerminatingState final
    {
        Handle handle_;
    };

    using State = std::variant<TerminatedState, StartingState, ReadyState, TerminatingState>;

    /// @brief Protects the state from concurrent modifications.
    mutable std::mutex state_mutex_;

    /// @brief Notified after a termination was handled.
    std::condition_variable termination_handled_;

    /// @brief The current state of the component.
    State state_;

    /// @brief One activation attempt, without cleaning up if it fails.
    /// @param stop_token Token which can be used to interrupt the wait for the ready condition.
    /// @return Whether the component activation succeeded, timed out or failed.
    [[nodiscard]] RequestResult tryActivate(cpp::stop_token stop_token);

    /// @brief First phase of the activation: start and track the resource, and change the state
    ///        from TerminatedState to StartingState.
    /// @details Locks the state. Afterwards, `activate` waits for the ready condition without
    ///          holding the lock, so that terminations can be reported in the meantime.
    /// @param stop_token Token which can be used to interrupt the activation.
    /// @return The handle of the started resource, or std::nullopt if it could not be started. The
    ///         state is still TerminatedState then.
    [[nodiscard]] std::optional<Handle> beginActivation(cpp::stop_token stop_token);

    /// @brief Last phase of the activation, after the wait for the ready condition: change the state
    ///        from StartingState to ReadyState, unless the activation failed.
    /// @details Locks the state. The stop token is not checked here, it only interrupts the wait.
    /// @param ready The result of the wait for the ready condition.
    /// @return Whether the component activation succeeded, timed out or failed.
    [[nodiscard]] RequestResult completeActivation(const Result<void>& ready);

    /// @brief Stop the resource after a failed activation and wait for its termination.
    /// @details Locks the state. Does nothing if no resource was started.
    /// @return True if nothing is left running, false if the resource could not be cleaned up.
    [[nodiscard]] bool stopAfterFailedActivation();

    /// @brief Stop the resource and wait for its termination, forcefully if needed.
    /// @details Always ends in TerminatedState. The lock is released while waiting.
    /// @pre @p lock holds the state lock.
    /// @param lock The lock on the state.
    /// @param handle The resource to stop.
    /// @return True if the resource is gone, false if it could not be cleaned up.
    [[nodiscard]] bool stopAndWait(std::unique_lock<std::mutex>& lock, const Handle& handle);

    /// @brief Wait until the termination of the resource was handled, or the timeout expired.
    /// @pre @p lock holds the state lock.
    /// @param lock The lock on the state, released while waiting.
    /// @param timeout The maximum duration to wait.
    /// @return True if the state left TerminatingState.
    [[nodiscard]] bool waitUntilTerminated(std::unique_lock<std::mutex>& lock, std::chrono::milliseconds timeout);

    /// @brief Handle the termination of the resource.
    /// @pre The state must be locked by the caller.
    /// @param status Exit code of the resource.
    /// @return Whether the termination is ignored or means the component failed.
    TerminationResult tryHandleTerminationInternally(int32_t status);

    /// @brief Return whether a termination with the given exit status is expected.
    /// @param status Exit code of the resource.
    /// @return True only for a self-terminating resource which exited with 0.
    [[nodiscard]] bool isExpectedTermination(int32_t status) const;

    /// @brief Start alive supervision now, if the component is supervised.
    /// @return The monotonic start time of the supervision, 0 for an unsupervised component, or
    ///         std::nullopt if supervision could not be activated.
    [[nodiscard]] std::optional<std::chrono::nanoseconds> activateSupervision() const;

    /// @brief Stop alive supervision now, if the component is supervised.
    [[nodiscard]] bool deactivateSupervision() const;
};

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_LCM_COMPONENT_HPP_INCLUDED
