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

#include "score/mw/lifecycle/ilm_control.hpp"

#include "score/mw/lifecycle/details/lm_control_impl.hpp"
#include "score/mw/lifecycle/execution_error.h"

// CreateLmControl<>() is instantiated here, so IWYU attributes the internals it uses to this file.
// IWYU pragma: no_include "score/language/safecpp/scoped_function/details/allocator_aware_type_erasure_pointer.h"
// IWYU pragma: no_include "score/language/safecpp/scoped_function/details/type_erasure_pointer.h"
// IWYU pragma: no_include "score/mw/com/impl/com_error.h"

namespace score::mw::lifecycle
{

score::Result<std::unique_ptr<ILmControl>> ILmControl::Create(std::string_view instance_specifier)
{
    return internal::CreateLmControl<>(instance_specifier);
}

}  // namespace score::mw::lifecycle
