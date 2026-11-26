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

#include "score/mw/launch_manager/common/init_only_resource.hpp"

#include <new>

namespace score::mw::lifecycle::internal
{

InitOnlyResource::InitOnlyResource(std::pmr::memory_resource* resource) noexcept : resource_(resource)
{
}

void InitOnlyResource::lock() noexcept
{
    locked_ = true;
}

void* InitOnlyResource::do_allocate(std::size_t bytes, std::size_t alignment)
{
    if (locked_)
    {
        throw std::bad_alloc();
    }

    return resource_->allocate(bytes, alignment);
}

void InitOnlyResource::do_deallocate(void* p, std::size_t bytes, std::size_t alignment)
{
    resource_->deallocate(p, bytes, alignment);
}

bool InitOnlyResource::do_is_equal(const std::pmr::memory_resource& other) const noexcept
{
    return this == &other;
}

}  // namespace score::mw::lifecycle::internal
