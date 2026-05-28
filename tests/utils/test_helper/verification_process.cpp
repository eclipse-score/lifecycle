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

#include "tests/utils/test_helper/test_helper.hpp"
#include <iostream>

int main()
{
    // This process must be active only in the fallback run target
    if (!touch_file(fallback_file))
    {
        std::cout << "Failed to write file!" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
