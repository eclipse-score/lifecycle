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
#include "score/hm/deadline/deadline_monitor.h"

extern "C" {
using namespace score::hm;
using namespace score::hm::internal;
using namespace score::hm::deadline;

// Deadline monitoring Foreign Function Interface Declarations that are exported by Rust implementation library
FFIHandle deadline_monitor_builder_create();
void deadline_monitor_builder_destroy(FFIHandle handle);
void deadline_monitor_builder_add_deadline(FFIHandle handler, const IdentTag* tag, uint32_t min, uint32_t max);
FFIError deadline_monitor_cpp_get_deadline(FFIHandle handler, const IdentTag* tag, FFIHandle* out);
void deadline_monitor_cpp_destroy(FFIHandle handler);
void deadline_destroy(FFIHandle deadline_handle);
FFIError deadline_start(FFIHandle deadline_handle);
void deadline_stop(FFIHandle deadline_handle);
}

// C++ wrapper for Rust library - the API implementation obeys the Rust API semantics and it's invariants

namespace score::hm::deadline
{
DeadlineMonitorBuilder::DeadlineMonitorBuilder()
    : monitor_builder_handler_(deadline_monitor_builder_create(), &deadline_monitor_builder_destroy)
{
}

DeadlineMonitorBuilder DeadlineMonitorBuilder::add_deadline(const IdentTag& tag, const TimeRange& range) &&
{
    auto handle = monitor_builder_handler_.as_rust_handle();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(handle.has_value());

    deadline_monitor_builder_add_deadline(handle.value(), &tag, range.min_ms(), range.max_ms());

    return std::move(*this);
}

DeadlineMonitor::DeadlineMonitor(FFIHandle handle) : monitor_handle_(handle, &deadline_monitor_cpp_destroy) {}

score::cpp::expected<Deadline, Error> DeadlineMonitor::get_deadline(const IdentTag& tag)
{
    auto handle = monitor_handle_.as_rust_handle();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(handle.has_value());

    FFIHandle ret = nullptr;
    auto result = deadline_monitor_cpp_get_deadline(handle.value(), &tag, &ret);

    if (result != kSuccess)
    {
        return score::cpp::unexpected(static_cast<Error>(result));
    }

    return score::cpp::expected<Deadline, Error>(Deadline{ret});
}

Deadline::Deadline(FFIHandle handle) : deadline_handle_(handle, &deadline_destroy), has_handle_(false) {}

Deadline::~Deadline()
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(!has_handle_);
}

score::cpp::expected<DeadlineHandle, Error> Deadline::start()
{
    // Cannot start a deadline that is already started
    if (has_handle_)
    {
        return score::cpp::unexpected(Error::WrongState);
    }

    auto handle = deadline_handle_.as_rust_handle();
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(handle.has_value());

    auto result = deadline_start(handle.value());
    if (result != kSuccess)
    {
        return score::cpp::unexpected(static_cast<Error>(result));
    }

    has_handle_ = true;
    return score::cpp::expected<DeadlineHandle, Error>(DeadlineHandle{*this});
}

DeadlineHandle::DeadlineHandle(Deadline& deadline) : was_stopped_(false), deadline_(deadline) {}

void DeadlineHandle::stop()
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION(deadline_.has_value());

    if (!was_stopped_)
    {
        was_stopped_ = true;
        auto handle = deadline_.value().get().deadline_handle_.as_rust_handle();
        SCORE_LANGUAGE_FUTURECPP_PRECONDITION(handle.has_value());

        deadline_stop(handle.value());
    }
}

DeadlineHandle::DeadlineHandle(DeadlineHandle&& other)
    : was_stopped_(other.was_stopped_), deadline_(std::move(other.deadline_))
{
    other.was_stopped_ = true;
    other.deadline_ = std::optional<std::reference_wrapper<Deadline>>{};  // None
}

DeadlineHandle::~DeadlineHandle()
{
    if (!deadline_.has_value())
    {
        return;
    }

    stop();
    deadline_.value().get().has_handle_ = false;
}

}  // namespace score::hm::deadline
