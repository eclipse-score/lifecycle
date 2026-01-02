// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef HM_DEADLINE_MONITOR_H
#define HM_DEADLINE_MONITOR_H

#include <cassert>
#include <expected>
#include "common.hpp"

struct hm_DeadlineMonitorBuilder;
struct hm_DeadlineMonitor;
struct hm_Deadline;

extern "C"
{
    hm_DeadlineMonitorBuilder *hm_dmb_new();
    void hm_dmb_delete(hm_DeadlineMonitorBuilder **builder);  // To be called only if hm_dmb_build wasn't called.
    void hm_dmb_add_deadline(hm_DeadlineMonitorBuilder *builder, hm_Tag tag, uint64_t min_ms, uint64_t max_ms);
    hm_Error hm_dmb_build(hm_DeadlineMonitorBuilder **builder, hm_DeadlineMonitor **out);

    void hm_dm_delete(hm_DeadlineMonitor **monitor);
    hm_Error hm_dm_get_deadline(hm_DeadlineMonitor *monitor, hm_Tag tag, hm_Deadline **out);
    hm_Error hm_dm_create_custom_deadline(hm_DeadlineMonitor *monitor, uint64_t min_ms, uint64_t max_ms, hm_Deadline **out);
    hm_Error hm_dm_enable(hm_DeadlineMonitor *monitor);
    hm_Error hm_dm_disable(hm_DeadlineMonitor *monitor);
    hm_Status hm_dm_status(const hm_DeadlineMonitor *monitor);

    void hm_dl_delete(hm_Deadline **deadline);
    hm_Error hm_dl_start(hm_Deadline *deadline);
    hm_Error hm_dl_stop(hm_Deadline *deadline);
    uint64_t hm_dl_min_ms(const hm_Deadline *deadline);
    uint64_t hm_dl_max_ms(const hm_Deadline *deadline);
}

namespace hm
{

struct Deadline
{
    Deadline(hm_Deadline *ptr) : ptr(ptr) {}

    Deadline(const Deadline &) = delete;
    Deadline &operator=(const Deadline &) = delete;

    Deadline(Deadline &&other) {
        this->ptr = other.ptr;
        other.ptr = nullptr;
    }

    Deadline &operator=(Deadline &&other) {
        this->ptr = other.ptr;
        other.ptr = nullptr;

        return *this;
    }

    ~Deadline() {
        if (this->ptr) {
            hm_dl_delete(&this->ptr);
        }
    }

    Error start() {
        return hm_dl_start(this->ptr);
    }

    Error stop() {
        return hm_dl_stop(this->ptr);
    }

    DurationRange range() const {
        return {
            .min = Duration(hm_dl_min_ms(this->ptr)),
            .max = Duration(hm_dl_max_ms(this->ptr))
        };
    }

    hm_Deadline *ffi_ptr() { return this->ptr; }

    const hm_Deadline *ffi_ptr() const { return this->ptr; }

private:
    hm_Deadline *ptr;
};

struct DeadlineMonitor
{
    DeadlineMonitor(hm_DeadlineMonitor *ptr) : ptr(ptr) {}

    DeadlineMonitor(const DeadlineMonitor &) = delete;
    DeadlineMonitor(DeadlineMonitor &&) = delete;
    DeadlineMonitor &operator=(const DeadlineMonitor &) = delete;
    DeadlineMonitor &operator=(DeadlineMonitor &&) = delete;

    ~DeadlineMonitor() { hm_dm_delete(&this->ptr); }

    std::expected<Deadline, Error> get_deadline(Tag tag)
    {
        hm_Deadline *ffi_dl_ptr;

        if (auto error = hm_dm_get_deadline(this->ptr, tag, &ffi_dl_ptr); error == Error::NoError) {
            return Deadline(ffi_dl_ptr);
        } else {
            return std::unexpected(error);
        }

    }

    std::expected<Deadline, Error> create_custom_deadline(DurationRange range)
    {
        hm_Deadline *ffi_dl_ptr;
        
        if (auto error = hm_dm_create_custom_deadline(this->ptr, range.min.count(), range.max.count(), &ffi_dl_ptr);
            error == Error::NoError)
        {
            return Deadline(ffi_dl_ptr);
        } else {
            return std::unexpected(error);
        }
    }

    Error enable() {
        return hm_dm_enable(this->ptr);
    }

    Error disable() {
        return hm_dm_disable(this->ptr);
    }

    Status status() const { return hm_dm_status(this->ptr); }

    hm_DeadlineMonitor *ffi_ptr() { return this->ptr; }

    const hm_DeadlineMonitor *ffi_ptr() const { return this->ptr; }

private:
    hm_DeadlineMonitor *ptr;
};

struct DeadlineMonitorBuilder
{
    DeadlineMonitorBuilder() : ptr(hm_dmb_new()) {}

    DeadlineMonitorBuilder(const DeadlineMonitorBuilder &) = delete;
    DeadlineMonitorBuilder(DeadlineMonitorBuilder &&) = delete;
    DeadlineMonitorBuilder &operator=(const DeadlineMonitorBuilder &) = delete;
    DeadlineMonitorBuilder &operator=(DeadlineMonitorBuilder &&) = delete;

    ~DeadlineMonitorBuilder()
    {
        if (this->ptr)
        {
            hm_dmb_delete(&this->ptr);
        }
    }

    DeadlineMonitorBuilder &add_deadline(Tag tag, DurationRange range)
    {
        hm_dmb_add_deadline(this->ptr, tag, range.min.count(), range.max.count());

        return *this;
    }

    DeadlineMonitor build()
    {
        assert(this->ptr);  // Ensure that build wasn't already called.

        hm_DeadlineMonitor *ffi_dm_ptr;
        hm_dmb_build(&this->ptr, &ffi_dm_ptr);

        return DeadlineMonitor(ffi_dm_ptr);
    }

    hm_DeadlineMonitorBuilder *ffi_ptr() { return this->ptr; }

    const hm_DeadlineMonitorBuilder *ffi_ptr() const { return this->ptr; }

private:
    hm_DeadlineMonitorBuilder *ptr;
};

}

#endif //HM_DEADLINE_MONITOR_H
