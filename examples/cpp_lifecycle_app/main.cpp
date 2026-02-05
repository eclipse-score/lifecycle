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

#include <optional>
#include <unistd.h>
#include <csignal>
#include <atomic>
#include <thread>
#include <iostream>
#include <ctime>
#include <string>
#include <chrono>
#include <vector>

#ifdef __linux__
    #include <linux/prctl.h>
    #include <sys/prctl.h>
#endif

#include "src/lifecycle_client_lib/include/application.h"
#include "src/lifecycle_client_lib/include/runapplication.h"

// ---------- Simple SIGTERM fallback ----------
namespace {
    std::atomic<bool> g_stopRequested{false};

    void onSignal(int) noexcept {
        g_stopRequested.store(true, std::memory_order_relaxed);
    }
}

struct Config {
    std::int32_t responseTimeInMs{100};
    bool         crashRequested{false};
    std::int32_t crashTimeInMs{1000};
    bool         failToStart{false};
    bool         verbose{false};
};

std::string helpSstring =
"Usage:\n\
       -r <response time in ms> Worst case response time to SIGTERM signal in milliseconds.\n\
       -c <crash time in ms> Simulate crash of the application, after specified time in milliseconds.\n\
       -s Simulate failure during start-up of the application.\n\
       -v Run in verbose mode.\n";

std::optional<Config> parseOptions(int argc, char *const *argv) noexcept
{
    Config config{};
    int c;
    while ((c = getopt(argc, argv, ":r:c:svh")) != -1)
    {
        switch (static_cast<char>(c))
        {
        case 'r':
            // response time
            config.responseTimeInMs = std::stoi(optarg);
            break;

        case 'c':
            // crash time
            config.crashRequested = true;
            config.crashTimeInMs = std::stoi(optarg);
            break;

        case 's':
            // start-up failure
            config.failToStart = true;
            break;

        case 'h':
            std::cout << helpSstring;
            return std::nullopt;

        case 'v':
            config.verbose = true;
            break;

        case '?':
            std::cout << "Unrecognized option: -" << static_cast<char>(optopt) << std::endl;
            std::cout << helpSstring;
            return std::nullopt;

        default:
            break;
        }
    }
    return config;
}

void set_process_name() {
    const char* identifier = getenv("PROCESSIDENTIFIER");
    if(identifier != nullptr) {
    #ifdef __QNXNTO__
            if (pthread_setname_np(pthread_self(), identifier) != 0) {
                std::cerr << "Failed to set QNX thread name" << std::endl;
            }
    #elif defined(__linux__)
            if (prctl(PR_SET_NAME, identifier) < 0) {
                std::cerr << "Failed to set process name to " << identifier << std::endl;
            }
    #endif
    }
}
class LifecycleApp final : public score::mw::lifecycle::Application {
public:
    std::int32_t Initialize(const score::mw::lifecycle::ApplicationContext& appCtx) override {
        std::signal(SIGINT,  onSignal);
        std::signal(SIGTERM, onSignal);
        set_process_name();
        
        const auto& argStrings = appCtx.get_arguments();

        m_argvStorage.clear();
        m_argvStorage.reserve(argStrings.size() + 2);

        if (argStrings.empty()) {
            // Minimal argv[0] so getopt never sees empty argv
            m_argvStorage.push_back(const_cast<char*>("LifecycleApp"));
        } else {
            for (const auto& s : argStrings) {
                m_argvStorage.push_back(const_cast<char*>(s.c_str()));
            }
        }

        m_argvStorage.push_back(nullptr);

        const int argcLocal = static_cast<int>(m_argvStorage.size() - 1);
        const auto cfgOpt = parseOptions(argcLocal, m_argvStorage.data());
        if (!cfgOpt.has_value()) {
            return EXIT_FAILURE;
        }

        m_cfg = *cfgOpt;
        if (m_cfg.failToStart) {
            return EXIT_FAILURE;
        }

        return 0;
    }

    std::int32_t Run(const score::cpp::stop_token& stopToken) override {
        const auto start = std::chrono::steady_clock::now();
        auto lastVerbose = start;

        timespec sleepReq{
            static_cast<time_t>(m_cfg.responseTimeInMs / 1000),
            static_cast<long>((m_cfg.responseTimeInMs % 1000) * 1000000L)
        };

        while (!stopToken.stop_requested() && !g_stopRequested.load(std::memory_order_relaxed)) {
            if (m_cfg.crashRequested) {
                const auto elapsedMs =
                    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

                const int remaining = static_cast<int>(m_cfg.crashTimeInMs - elapsedMs);
                if (remaining < m_cfg.responseTimeInMs) {
                    if (remaining > 0) {
                        timespec crashReq{
                            static_cast<time_t>(remaining / 1000),
                            static_cast<long>((remaining % 1000) * 1000000L)
                        };
                        nanosleep(&crashReq, nullptr);
                    }
                    std::abort();
                }
            }

            if (m_cfg.verbose) {
                const auto now = std::chrono::steady_clock::now();
                if (now - lastVerbose >= std::chrono::seconds(1)) {
                    std::cout << "LifecycleApp: Running in verbose mode\n";
                    lastVerbose = now;
                }
            }

            nanosleep(&sleepReq, nullptr);
        }

        return EXIT_SUCCESS;
    }

private:
    Config m_cfg{};
    std::vector<char*> m_argvStorage; // keeps argv stable for getopt()
};

int main(int argc, char** argv) {
    return score::mw::lifecycle::run_application<LifecycleApp>(argc, argv);
}
