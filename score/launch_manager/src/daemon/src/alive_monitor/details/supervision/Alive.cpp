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

#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"

#include <score/assert.hpp>
#include <string_view>

#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"

namespace score::mw::lifecycle::internal::saf::supervision
{

Alive::Alive(
    const IdentifierHash id,
    const ComponentAliveSupervision& f_aliveCfg_r,
    const std::shared_ptr<IRecoveryClient> recovery_client,
    common::Observable<ifappl::Checkpoint>& interface,
    const uint16_t bufferSize)
    : ISupervision(id),
      alive_reference_cycle_(
          timers::TimeConversion::convertMilliSecToNanoSec(std::chrono::milliseconds{f_aliveCfg_r.reporting_cycle_ms})),
      min_alive_indications_(f_aliveCfg_r.min_indications.value_or(0)),
      max_alive_indications_(f_aliveCfg_r.max_indications.value_or(0)),
      is_min_check_disabled_(min_alive_indications_ == 0),
      is_max_check_disabled_(max_alive_indications_ == 0),
      failed_supervision_cycles_tolerance_(f_aliveCfg_r.failed_cycles_tolerance),
      recovery_client_(recovery_client),
      process_identifier_(id),
      time_sorting_update_event_buffer_(common::TimeSortingBuffer<TimeSortedUpdateEvent>(bufferSize))
{
    interface.attachObserver(*this);
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(
        (alive_reference_cycle_.count() != 0U), "alive_reference_cycle_=0 causes infinite loop during evaluation.");

    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(
        (alive_status_ == EStatus::kDeactivated), "Alive Supervision must start in deactivated state");

    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(
        (recovery_client_ != nullptr), "Recovery client must be provided");
}

void Alive::updateData(const score::mw::lifecycle::internal::saf::ifappl::Checkpoint& f_observable_r) noexcept(true)
{
    std::chrono::nanoseconds timestamp{f_observable_r.timestamp};

    if (f_observable_r.isDataLossEvent)
    {
        data_loss_reason_ = EDataLossReason::kSharedMemory;
        // If clock error is detected, last syncTimestamp is used as event timestamp.
        event_timestamp_ = ((timestamp.count() == 0U) ? last_sync_timestamp_ : timestamp);
    }
    else
    {
        CheckpointSnapshot checkpointSnapshot{timestamp};
        if (!time_sorting_update_event_buffer_.push(checkpointSnapshot, timestamp))
        {
            data_loss_reason_ = EDataLossReason::kBufferFull;
            event_timestamp_ = last_sync_timestamp_;
        }
    }
}

void Alive::updateData(const ifexm::ObservableEvent& f_observable_r) noexcept(true)
{
    const std::chrono::nanoseconds timestamp{
        timers::TimeConversion::convertToNanoSec(f_observable_r.event.systemClockTimestamp)};
    SupervisionEventSnapshot snapshot{timestamp, f_observable_r.event.eventType};
    if (!time_sorting_update_event_buffer_.push(snapshot, timestamp))
    {
        data_loss_reason_ = EDataLossReason::kBufferFull;
        event_timestamp_ = last_sync_timestamp_;
    }
}

Alive::EStatus Alive::getStatus(void) const noexcept(true)
{
    return alive_status_;
}

std::chrono::nanoseconds Alive::getTimestamp(void) const noexcept(true)
{
    return event_timestamp_;
}

void Alive::evaluate(const std::chrono::nanoseconds f_syncTimestamp)
{
    storeSyncEvent(f_syncTimestamp);

    if (data_loss_reason_ != EDataLossReason::kNoDataLoss)
    {
        handleDataLossReaction();
        last_sync_timestamp_ = f_syncTimestamp;
        return;
    }

    // Scan individual checkpoint/event from history buffer and update alive status
    TimeSortedUpdateEvent* sortedUpdateEvent_p{time_sorting_update_event_buffer_.getNextElement()};

    while (sortedUpdateEvent_p != nullptr)
    {
        std::chrono::nanoseconds timestampOfUpdateEvent{getTimestampOfUpdateEvent(*sortedUpdateEvent_p)};
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            (timestampOfUpdateEvent <= f_syncTimestamp),
            "Alive supervision: Checkpoint events are reported beyond syncTimestamp.");

        // Check if evaluation is to be triggered before processing current sorted update event
        const bool isEvaluationEvent{detectEvaluationEvent(timestampOfUpdateEvent, *sortedUpdateEvent_p)};
        if (isEvaluationEvent)
        {
            timestampOfUpdateEvent = reference_cycle_end_;
            event_timestamp_ = reference_cycle_end_;
        }

        EUpdateEventType currentUpdateType{getAliveEventType(isEvaluationEvent, *sortedUpdateEvent_p)};

        switch (alive_status_)
        {
            case EStatus::kDeactivated:
            {
                checkTransitionsOutOfDeactivated(currentUpdateType, timestampOfUpdateEvent);
                break;
            }

            case EStatus::kOk:
            {
                checkTransitionsOutOfOk(currentUpdateType, timestampOfUpdateEvent);
                break;
            }

            case EStatus::kFailed:
            {
                checkTransitionsOutOfFailed(currentUpdateType, timestampOfUpdateEvent);
                break;
            }

            case EStatus::kExpired:
            {
                // EStatus::kExpired can only be exited with a switch to deactivated.
                // A common switch to deactivation is handled in the end, therefore nothing additionally has to be
                // done for this state.
                break;
            }

            default:
            {
                event_timestamp_ = last_sync_timestamp_;
                switchToExpired(EReason::kDataCorruption);
                break;
            }
        }

        checkTransitionsToDeactivated(currentUpdateType, timestampOfUpdateEvent);

        // If evaluation event is triggered in this iteration, original event is passed to next iteration.
        if (!(isEvaluationEvent))
        {
            sortedUpdateEvent_p = time_sorting_update_event_buffer_.getNextElement();
        }
    }

    time_sorting_update_event_buffer_.clear();
    last_sync_timestamp_ = f_syncTimestamp;
}

void Alive::storeSyncEvent(const std::chrono::nanoseconds f_syncTimestamp)
{
    // If there is a reported alive checkpoint exactly at syncTimestamp, push will update sync event after the
    // reported alive checkpoint during sorting. Reason: Sync event is pushed after last alive checkpoint.
    SyncSnapshot syncSnapshot{f_syncTimestamp};
    if (!time_sorting_update_event_buffer_.push(syncSnapshot, f_syncTimestamp))
    {
        data_loss_reason_ = EDataLossReason::kBufferFull;
        event_timestamp_ = last_sync_timestamp_;
    }
}

void Alive::handleDataLossReaction(void) noexcept(true)
{
    // In case of data loss event, state transition from deactivated to expired is accepted.
    if (EStatus::kExpired != alive_status_)
    {
        switchToExpired(EReason::kDataLoss);
    }
    time_sorting_update_event_buffer_.clear();
    data_loss_reason_ = EDataLossReason::kNoDataLoss;
}

bool Alive::detectEvaluationEvent(
    const std::chrono::nanoseconds f_timestampOfUpdateEvent,
    const TimeSortedUpdateEvent f_updateEvent) const noexcept(true)
{
    bool isEvaluationEvent;

    // Hint: EvaluationEvent can be triggered only if current state is ok or failed.
    // If current state is deactivated or expired, referenceCyclEnd is reset to the highest value. This means that
    // there is no need of evaluation.

    if ((alive_status_ == EStatus::kDeactivated) || (alive_status_ == EStatus::kExpired))
    {
        return false;
    }

    // Case 1: Evaluation event exists: If reference_cycle_end_ exists before current checkpoint event or observable
    // event event, trigger evaluation event for assessing alive checkpoints

    // Hint 1: If there are multiple reference cycles within single daemon cycle, reference_cycle_end_ must increase
    // after each evaluation event. Otherwise infinite loop occurs.
    // To avoid infinite loop, ensure:
    //      1. setNextCycle() is called during each evaluation event
    //      2. alive_reference_cycle_ != 0

    // Hint 2: Condition "(reference_cycle_end_ <= f_timestampOfUpdateEvent)" (not coded here) will push the current
    // alive checkpoint (which is exactly at reference_cycle_end_) after evlaluation event is triggered. This behavior
    // will miss to count last alive checkpoint at reference_cycle_end_. This is avoided by splitting the condition
    // ">=" into ">" and "==" for correct behavior.

    // Hint 3: Condition "(reference_cycle_end_ == f_timestampOfUpdateEvent) && <isSync>" is introduced to consider
    // a case in which last checkpoint and sync event align with reference cycle end. In this case, evaluation event
    // is triggered only after considering last alive checkpoint event which occurs at reference cycle end and sync
    // event.

    // Hint 4: Sync event is placed at the end of the buffer. The last alive checkpoint event which aligns with both
    // reference cycle end and sync event is placed before sync event.

    if ((reference_cycle_end_ < f_timestampOfUpdateEvent) ||
        ((reference_cycle_end_ == f_timestampOfUpdateEvent) && std::holds_alternative<SyncSnapshot>(f_updateEvent)))
    {
        isEvaluationEvent = true;
    }

    // Case 2: Evaluation event does not exist: If reference_cycle_end_ exists after current checkpoint event or
    // observable event event, consider current event for activation/deactivation/counting in next steps. Do not
    // trigger evaluation event for this case.

    else
    {
        isEvaluationEvent = false;
    }

    return isEvaluationEvent;
}

Alive::EUpdateEventType Alive::getAliveEventType(
    bool f_isEvaluationEvent,
    const TimeSortedUpdateEvent f_updateEvent) noexcept(true)
{
    if (f_isEvaluationEvent)
    {
        return EUpdateEventType::kEvaluation;
    }

    if (std::holds_alternative<SupervisionEventSnapshot>(f_updateEvent))
    {
        const auto& snapshot = std::get<SupervisionEventSnapshot>(f_updateEvent);
        if (snapshot.eventType == score::mw::lifecycle::SupervisionEventType::kActivation)
        {
            return EUpdateEventType::kActivation;
        }
        if (snapshot.eventType == score::mw::lifecycle::SupervisionEventType::kDeactivation)
        {
            return EUpdateEventType::kDeactivation;
        }
        return EUpdateEventType::kNoChange;
    }

    if (std::holds_alternative<CheckpointSnapshot>(f_updateEvent))
    {
        return EUpdateEventType::kCheckpoint;
    }

    // SyncSnapshot
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(std::holds_alternative<SyncSnapshot>(f_updateEvent));
    return EUpdateEventType::kSync;
}

void Alive::checkTransitionsOutOfDeactivated(
    const EUpdateEventType f_updateEventType,
    const std::chrono::nanoseconds f_updateEventTimestamp) noexcept(true)
{
    if (f_updateEventType == EUpdateEventType::kActivation)
    {
        if (!setReferenceCycleTimestamps(f_updateEventTimestamp))
        {
            event_timestamp_ = f_updateEventTimestamp;
            switchToOk();
        }
    }
}

void Alive::checkTransitionsToDeactivated(
    const EUpdateEventType f_updateEventType,
    const std::chrono::nanoseconds f_updateEventTimestamp) noexcept(true)
{
    if ((f_updateEventType == EUpdateEventType::kDeactivation) && (alive_status_ != EStatus::kDeactivated))
    {
        event_timestamp_ = f_updateEventTimestamp;
        switchToDeactivated();
    }
}

void Alive::checkTransitionsOutOfOk(
    const EUpdateEventType f_updateEventType,
    const std::chrono::nanoseconds f_updateEventTimestamp) noexcept(true)
{
    // Accept only alive checkpoint or evaluation event.
    // Deactivation event is handled at the end of evaluate function.
    if (f_updateEventType == EUpdateEventType::kEvaluation)
    {
        evaluateRefCycleOutOfOk();
    }
    else if (f_updateEventType == EUpdateEventType::kCheckpoint)
    {
        incIndicationCount(f_updateEventTimestamp);
    }
    else
    {
        // ignore
    }
}

bool Alive::setReferenceCycleTimestamps(std::chrono::nanoseconds f_baseValue) noexcept(true)
{
    if (f_baseValue > std::chrono::nanoseconds::max() - alive_reference_cycle_)
    {
        LM_LOG_ERROR() << "Alive Supervision (" << getConfigName()
                       << ") overflow appeared during increase of reference cycle timestamps";
        event_timestamp_ = f_baseValue;
        switchToExpired(EReason::kOverflow);
        return true;
    }
    reference_cycle_start_ = f_baseValue;
    reference_cycle_end_ = reference_cycle_start_ + alive_reference_cycle_;
    return false;
}

void Alive::incIndicationCount(const std::chrono::nanoseconds f_updateEventTimestamp) noexcept(true)
{
    if (indication_count_ == UINT32_MAX)
    {
        LM_LOG_ERROR() << "Alive Supervision (" << getConfigName()
                       << ") overflow appeared during incrementation of indication count";
        event_timestamp_ = f_updateEventTimestamp;
        return switchToExpired(EReason::kOverflow);
    }
    indication_count_++;
}

void Alive::evaluateRefCycleOutOfOk(void) noexcept(true)
{
    if (isMinError() || isMaxError())
    {
        if (failed_supervision_cycles_ < failed_supervision_cycles_tolerance_)
        {
            switchToFailed();
        }
        else
        {
            switchToExpired(EReason::kFailedToleranceExceeded);
        }
    }
    else
    {
        // No error stay in state OK
        setNextCycle();
    }
}

void Alive::checkTransitionsOutOfFailed(
    const EUpdateEventType f_updateEventType,
    const std::chrono::nanoseconds f_updateEventTimestamp) noexcept(true)
{
    // Accept only alive checkpoint or evaluation event.
    // Deactivation event is handled at the end of evaluate function.
    if (f_updateEventType == EUpdateEventType::kEvaluation)
    {
        evaluateRefCycleOutOfFailed();
    }
    else if (f_updateEventType == EUpdateEventType::kCheckpoint)
    {
        incIndicationCount(f_updateEventTimestamp);
    }
    else
    {
        // ignore
    }
}

void Alive::evaluateRefCycleOutOfFailed(void) noexcept(true)
{
    if (isMinError() || isMaxError())
    {
        if (failed_supervision_cycles_ == UINT32_MAX)
        {
            LM_LOG_ERROR() << "Alive Supervision (" << getConfigName()
                           << ") overflow appeared during incrementation of failed supervision cycles";
            switchToExpired(EReason::kOverflow);
            return;
        }
        ++failed_supervision_cycles_;
        if (failed_supervision_cycles_ <= failed_supervision_cycles_tolerance_)
        {
            logExpiredFailedStateDetails();
            setNextCycle();
        }
        else
        {
            switchToExpired(EReason::kFailedToleranceExceeded);
        }
    }
    else
    {
        if (failed_supervision_cycles_ <= 1U)
        {
            switchToOk();
        }
        else
        {
            failed_supervision_cycles_--;
        }
        setNextCycle();
    }
}

void Alive::switchToDeactivated(void) noexcept(true)
{
    alive_status_ = EStatus::kDeactivated;
    failed_supervision_cycles_ = 0U;
    indication_count_ = 0U;
    reference_cycle_start_ = std::chrono::nanoseconds{0U};
    reference_cycle_end_ = std::chrono::nanoseconds::max();

    LM_LOG_DEBUG() << "Alive Supervision (" << getConfigName() << ") switched to DEACTIVATED.";

    pushResultToObservers(*this);
}

void Alive::switchToOk(void) noexcept(true)
{
    alive_status_ = EStatus::kOk;
    failed_supervision_cycles_ = 0U;
    LM_LOG_INFO() << "Alive Supervision (" << getConfigName() << ") switched to OK.";
    pushResultToObservers(*this);
}

void Alive::switchToFailed(void) noexcept(true)
{
    alive_status_ = EStatus::kFailed;
    // Method caller is responsible for preventing overflow
    failed_supervision_cycles_++;

    logExpiredFailedStateDetails();
    pushResultToObservers(*this);
    setNextCycle();
}

void Alive::switchToExpired(Alive::EReason reason) noexcept(true)
{
    alive_status_ = EStatus::kExpired;

    switch (reason)
    {
        case EReason::kDataLoss:
            switch (data_loss_reason_)
            {
                case EDataLossReason::kSharedMemory:
                    LM_LOG_ERROR() << "Alive Supervision (" << getConfigName()
                                   << ") switched to EXPIRED, due to shared memory overflow.";
                    break;
                case EDataLossReason::kBufferFull:
                    LM_LOG_ERROR() << "Alive Supervision (" << getConfigName()
                                   << ") switched to EXPIRED, due to buffer overflow.";
                    break;
                default:
                    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(data_loss_reason_ != EDataLossReason::kNoDataLoss);
                    LM_LOG_ERROR() << "Alive Supervision (" << getConfigName()
                                   << ") switched to EXPIRED, due to unknown data loss case.";
                    break;
            }
            break;
        case EReason::kFailedToleranceExceeded:
        {
            logExpiredFailedStateDetails();
            break;
        }
        case EReason::kOverflow:
        {
            LM_LOG_ERROR() << "Alive Supervision (" << getConfigName() << ") switched to EXPIRED, due to overflow.";
            break;
        }
        default:
            LM_LOG_WARN() << "Alive Supervision (" << getConfigName() << ") switched to EXPIRED";
            break;
    }

    failed_supervision_cycles_ = failed_supervision_cycles_tolerance_;
    indication_count_ = 0U;
    reference_cycle_start_ = std::chrono::nanoseconds{0U};
    reference_cycle_end_ = std::chrono::nanoseconds::max();
    data_loss_reason_ = EDataLossReason::kNoDataLoss;

    const bool enqueued = recovery_client_->sendRecoveryRequest(process_identifier_, event_timestamp_);
    if (enqueued)
    {
        LM_LOG_DEBUG() << "Recovery request enqueued successfully for alive supervision" << getConfigName()
                       << "failure";
    }
    else
    {
        LM_LOG_ERROR() << "Failed to enqueue recovery request for alive supervision" << getConfigName()
                       << "failure (ring buffer full)";
        recovery_enqueue_failed_ = true;
    }

    pushResultToObservers(*this);
}

bool Alive::hasRecoveryEnqueueFailed(void) const noexcept
{
    return recovery_enqueue_failed_;
}

void Alive::setNextCycle(void) noexcept(true)
{
    if (!setReferenceCycleTimestamps(reference_cycle_end_))
    {
        indication_count_ = 0U;
    }
}

bool Alive::isMinError(void) const noexcept(true)
{
    return ((is_min_check_disabled_ == false) && (indication_count_ < min_alive_indications_));
}

bool Alive::isMaxError(void) const noexcept(true)
{
    return ((is_max_check_disabled_ == false) && (indication_count_ > max_alive_indications_));
}

void Alive::logExpiredFailedStateDetails() const noexcept(true)
{
    std::string_view failedState{""};
    if (alive_status_ == EStatus::kFailed)
    {
        // failed_supervision_cycles_ == 1 if just switched to FAILED and > 1 if were already in FAILED before
        failedState = (failed_supervision_cycles_ > 1U) ? "next cycle FAILED" : "switched to FAILED";
    }
    if (alive_status_ == EStatus::kExpired)
    {
        failedState = "switched to EXPIRED";
    }

    const bool minError{isMinError()};
    const std::uint64_t aliveIndicationMargin{minError ? min_alive_indications_ : max_alive_indications_};
    const std::string_view expectedComparison{minError ? ">=" : "<="};
    LM_LOG_WARN() << "Alive Supervision (" << getConfigName() << ")" << failedState << ", due to" << indication_count_
                  << "reported alive indication(s) (expected" << expectedComparison << aliveIndicationMargin
                  << "). Failed supervision cycles:" << failed_supervision_cycles_ << "/"
                  << failed_supervision_cycles_tolerance_;
}

std::chrono::nanoseconds Alive::getTimestampOfUpdateEvent(const TimeSortedUpdateEvent f_updateEvent) noexcept(true)
{
    std::chrono::nanoseconds timestamp{0U};
    if (std::holds_alternative<SupervisionEventSnapshot>(f_updateEvent))
    {
        timestamp = std::get<SupervisionEventSnapshot>(f_updateEvent).timestamp;
    }
    else if (std::holds_alternative<CheckpointSnapshot>(f_updateEvent))
    {
        timestamp = std::get<CheckpointSnapshot>(f_updateEvent).timestamp;
    }
    else
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(std::holds_alternative<SyncSnapshot>(f_updateEvent));
        timestamp = std::get<SyncSnapshot>(f_updateEvent);
    }

    return timestamp;
}

IdentifierHash Alive::getIdentifier() const noexcept
{
    return getConfigName();
}

}  // namespace score::mw::lifecycle::internal::saf::supervision
