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

#ifndef SCORE_HM_TAG_H
#define SCORE_HM_TAG_H

#include <cstddef>

namespace score::hm
{

/// Common string-based tag.
template <typename T>
class Tag
{
  public:
    /// Create a new tag from a C-style string.
    template <size_t N>
    explicit Tag(const char (&tag)[N]) : data_(tag), length_(N - 1)
    {
    }

  private:
    /// SAFETY: This has to be FFI compatible with the Rust side representation.
    const char* const data_;
    size_t length_;
};

/// Monitor tag.
class MonitorTag : public Tag<MonitorTag>
{
  public:
    /// Create a new MonitorTag from a C-style string.
    template <size_t N>
    explicit MonitorTag(const char (&tag)[N]) : Tag{tag}
    {
    }
};

/// Deadline tag.
class DeadlineTag : public Tag<DeadlineTag>
{
  public:
    /// Create a new DeadlineTag from a C-style string.
    template <size_t N>
    explicit DeadlineTag(const char (&tag)[N]) : Tag{tag}
    {
    }
};

}  // namespace score::hm

#endif  // SCORE_HM_TAG_H
