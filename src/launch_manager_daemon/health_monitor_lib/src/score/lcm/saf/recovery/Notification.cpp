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

#include "score/lcm/saf/recovery/Notification.hpp"

namespace score
{
namespace lcm
{
namespace saf
{
namespace recovery
{

Notification::Notification(std::shared_ptr<score::lcm::IRecoveryClient> f_recoveryClient_r) :
    currentState(State::kIdle),
    messageHeader("Notification ( / )"),
    isNotificationConfigAvailable(false),
    recoveryClient(f_recoveryClient_r),
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::recovery))
{
}

Notification::Notification(const NotificationConfig& f_notificationConfig_r, std::shared_ptr<score::lcm::IRecoveryClient> f_recoveryClient_r) :
    currentState(State::kIdle),
    k_notificationConfig(f_notificationConfig_r),
    messageHeader("Notification (" + k_notificationConfig.configName + ")"),
    isNotificationConfigAvailable(true),
    recoveryClient(f_recoveryClient_r),
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::recovery))
{
}

// explicit declaration of destructor needed to make forward declaration work
Notification::~Notification() = default;

/* RULECHECKER_comment(0, 7, check_min_instructions, "Default move constructor is not provided\
a function body", true_no_defect) */
/* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
the member initializer", false) */
/* RULECHECKER_comment(0, 3, check_copy_in_move_constructor, "The default move constructor invokes parameterized\
constructor internally. This invokes std::string copy construction", true_no_defect) */
Notification::Notification(Notification&&) = default;

bool Notification::initProxy() noexcept(false)
{
    if(!isNotificationConfigAvailable) {
        return true;
    }

    const auto processGroupStateStartPos = k_notificationConfig.processGroupMetaModelIdentifier.find_last_of('/');
    if(processGroupStateStartPos == std::string::npos || processGroupStateStartPos == 0U)
    {
        logger_r.LogError() << messageHeader << "Invalid ProcessGroupState identifier:"
                            << k_notificationConfig.processGroupMetaModelIdentifier;

        return false;
    }

    const std::string recoveryProcessGroupId{k_notificationConfig.processGroupMetaModelIdentifier.substr(0, processGroupStateStartPos)};
    recoveryProcessGroup = score::lcm::IdentifierHash(recoveryProcessGroupId);
    return true;
}

void Notification::send(const xaap::lcm::saf::recovery::supervision::SupervisionErrorInfo& f_executionErrorInfo_r)
{
    static_cast<void>(f_executionErrorInfo_r); // Unused, to be removed in the future together with the Notification class

    if (isNotificationConfigAvailable)
    {
        if (currentState == State::kIdle)
        {
            currentState = State::kSending;
        }
    } else {
        currentState = State::kTimeout;
    }
}

void Notification::cyclicTrigger(void)
{
    if (currentState == State::kSending)
    {
        invokeRecoveryHandler();
    }
}

bool Notification::isFinalTimeoutStateReached(void) const noexcept
{
    return (currentState == State::kTimeout);
}

void Notification::invokeRecoveryHandler(void)
{
    // TODO: As a next step, we shall pass the Id of the Process which failed supervision
    // instead of the process group id. However, this required other refactoring first.
    const bool enqueued = recoveryClient->sendRecoveryRequest(recoveryProcessGroup);

    if (enqueued)
    {
        logger_r.LogInfo() << messageHeader << "Recovery request enqueued successfully";
        currentState = State::kIdle;
    }
    else
    {
        logger_r.LogError() << messageHeader << "Failed to enqueue recovery request (ring buffer full)";
        currentState = State::kTimeout;
    }
}

/* RULECHECKER_comment(1:0,2:0, check_min_instructions, "False positive", false) */
/* RULECHECKER_comment(1:0,1:0, check_member_function_missing_static, "Member function uses non-static members and\
cannot be made static", false) */
const std::string& Notification::getConfigName(void) const noexcept
{
    return k_notificationConfig.configName;
}

}  // namespace recovery
}  // namespace saf
}  // namespace lcm
}  // namespace score
