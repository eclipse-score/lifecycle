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

#include <cstdlib>

#include <score/mw/lifecycle/application.h>
#include <score/mw/lifecycle/runapplication.h>

class LifecycleApp final : public score::mw::lifecycle::Application
{
  public:
    // called before the process reports Running to the Launch Manager
    std::int32_t Initialize(const score::mw::lifecycle::ApplicationContext& /*appCtx*/) override
    {
        // read arguments and set up resources here
        return EXIT_SUCCESS;
    }

    // called once initialization succeeded, returns when termination is requested
    std::int32_t Run(const score::cpp::stop_token& stopToken) override
    {
        while (!stopToken.stop_requested())
        {
            // do some work
        }

        return EXIT_SUCCESS;
    }
};

int main(int argc, char** argv)
{
    return score::mw::lifecycle::run_application<LifecycleApp>(argc, argv);
}
