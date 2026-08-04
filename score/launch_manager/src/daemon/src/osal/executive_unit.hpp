#ifndef OSAL_EXECUTION_UNIT_HPP_INCLUDED
#define OSAL_EXECUTION_UNIT_HPP_INCLUDED

#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/launch_manager/osal/ipc_comms.hpp"
#include "score/mw/launch_manager/common/constants.hpp"
#include <string>
#include <array>
#include <vector>
#include <cstdint>

namespace score {

namespace lcm {

namespace internal {

namespace osal {

/// @brief Specification for starting an execution unit.
/// This is a superset of today's OsalConfig, supporting native processes,
/// OCI containers, and other runtime backends.
struct ExecutionSpec {
    // Common fields
    std::string short_name_{};  ///< Short name of the component
    
    CommsType comms_type_{CommsType::kNoComms};  ///< Communication type required
    
    // Native process fields (also used by OCI when crun is the runtime)
    std::string executable_path_{};                                     ///< Path to the executable
    std::array<const char*, score::lcm::internal::kArgvArraySize> argv_{};  ///< Command-line arguments
    char* envp_[score::lcm::internal::kEnvArraySize];                      ///< Environment variables
    
    // OCI-specific fields (for future OCI container support)
    std::string bundle_path_{};  ///< Path to OCI bundle directory
    std::string runtime_path_{};  ///< Path to OCI runtime (e.g., /usr/bin/crun)
    
    // Security and resource fields
    uid_t uid_{0};                              ///< User ID
    gid_t gid_{0};                              ///< Group ID
    std::vector<gid_t> supplementary_gids_{};   ///< Supplementary group IDs
    uint32_t cpu_mask_{0};                      ///< Mask for setting processor core affinity
    int32_t scheduling_policy_{0};              ///< Scheduling policy
    int32_t scheduling_priority_{0};            ///< Scheduling priority
    
    // Resource limits (from OsalLimits)
    uint64_t max_memory_usage_{0};      ///< Maximum memory usage in bytes (heapUsage)
    uint64_t max_address_space_{0};     ///< Maximum address space usage in bytes (systemMemoryUsage)
    uint64_t max_stack_usage_{0};       ///< Maximum stack usage in bytes (resource group memUsage)
    uint64_t max_cpu_usage_{0};         ///< Maximum cpu usage in seconds (resource group cpuUsage)
};

/// @brief Interface for an execution unit backend.
/// This is the pluggable abstraction that allows Launch Manager to manage
/// components backed by native processes, OCI containers, or delegated runtimes.
class IExecutionUnit {
   public:
    virtual ~IExecutionUnit() = default;
    
    /// @brief Start an execution unit.
    /// @param spec Specification for the execution unit
    /// @param out Output parameter that receives the opaque handle on success
    /// @param comms Optional pointer to IPC comms object (may be nullptr)
    /// @return kSuccess on success, kFail or kTimeout on failure
    virtual OsalReturnType start(const ExecutionSpec& spec,
                                ExecutionHandle& out,
                                IpcCommsP* comms) = 0;
    
    /// @brief Request graceful termination of an execution unit.
    /// @param handle Handle of the execution unit to terminate
    /// @return kSuccess on success, kFail on failure
    virtual OsalReturnType requestTermination(const ExecutionHandle& handle) = 0;
    
    /// @brief Forcefully terminate an execution unit.
    /// @param handle Handle of the execution unit to terminate
    /// @return kSuccess on success, kFail on failure
    virtual OsalReturnType forceTermination(const ExecutionHandle& handle) = 0;
    
    /// @brief Get the death source for this execution unit backend.
    /// The death source is used by OsHandler to monitor for termination events.
    /// @return Reference to the IDeathSource interface
    virtual IDeathSource& deathSource() = 0;
};

/// @brief Interface for a death notification source.
/// This allows OsHandler to multiplex death notifications from multiple backends
/// (native processes, OCI containers, delegated runtimes).
class IDeathSource {
   public:
    virtual ~IDeathSource() = default;
    
    /// @brief Get a pollable file descriptor that becomes readable when the unit dies.
    /// This fd is registered in epoll/poll sets for event-driven monitoring.
    /// @return File descriptor suitable for polling
    virtual int pollable_fd() const = 0;
    
    /// @brief Drain death events from this source.
    /// When pollable_fd() indicates readiness, this method translates the OS/runtime
    /// event into (handle, status) pairs.
    /// @param out Vector to receive death events
    /// @return true if events were drained, false if no events available
    virtual bool drain(std::vector<DeathEvent>& out) = 0;
};

}  // namespace osal

}  // namespace lcm

}  // namespace internal

}  // namespace score

#endif  // OSAL_EXECUTION_UNIT_HPP_INCLUDED