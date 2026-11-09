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

#ifndef INIT_ONLY_RESOURCE_HPP_INCLUDED
#define INIT_ONLY_RESOURCE_HPP_INCLUDED

#include <cstddef>
#include <memory_resource>

namespace score::mw::lifecycle::internal
{

/// @brief Memory resource which forwards to the heap until it is locked,
///        then disallows any further allocations.
class InitOnlyResource final : public std::pmr::memory_resource
{
  public:
    /// @brief Disallow any further allocations.
    void lock() noexcept;

  private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override;
    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override;
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

    bool locked_ = false;
};

}  // namespace score::mw::lifecycle::internal

#endif  /// INIT_ONLY_RESOURCE_HPP_INCLUDED
