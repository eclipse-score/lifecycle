#ifndef OSAL_NATIVE_EXECUTION_UNIT_HPP_INCLUDED
#define OSAL_NATIVE_EXECUTION_UNIT_HPP_INCLUDED

#include "score/mw/launch_manager/osal/executive_unit.hpp"
#include "score/mw/launch_manager/osal/return_types.hpp"
#include "score/mw/launch_manager/process_group_manager/iprocess.hpp"
#include "score/mw/launch_manager/process_group_manager/details/safe_process_map.hpp"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace score {

namespace lcm {

namespace internal {

namespace osal {

/// @brief Native process execution unit implementation.
/// This class wraps the existing IProcess interface to provide a unified
/// ExecutionUnit abstraction for native processes.
class NativeExecutionUnit final : public IExecutionUnit, public IDeathSource, public ITerminationCallback {
   public:
    /// @brief Constructor
    NativeExecutionUnit();
    
    /// @brief Destructor
    ~NativeExecutionUnit() override;
    
    // IExecutionUnit interface
    OsalReturnType start(const ExecutionSpec& spec,
                        ExecutionHandle& out,
                        IpcCommsP* comms) override;
    
    OsalReturnType requestTermination(const ExecutionHandle& handle) override;
    
    OsalReturnType forceTermination(const ExecutionHandle& handle) override;
    
    IDeathSource& deathSource() override {
        return *this;
    }
    
    // IDeathSource interface
    int pollable_fd() const override;
    
    bool drain(std::vector<DeathEvent>& out) override;
    
    // ITerminationCallback interface
    void terminated(int32_t process_status) override;
    
    /// @brief Set the SafeProcessMap instance for process tracking.
    static void setSafeProcessMap(SafeProcessMap* map);
    
   private:
    /// @brief Generate a unique token for an execution handle.
    std::uint64_t generateToken();
    
    /// @brief Map from execution handle token to process information.
    struct ProcessInfo {
        ProcessID pid;
        int32_t status;
    };
    std::unordered_map<std::uint64_t, ProcessInfo> process_map_;
    
    /// @brief Mutex protecting the process map.
    mutable std::mutex process_map_mutex_;
    
    /// @brief Counter for generating unique tokens.
    std::atomic_uint64_t token_counter_{0};
    
    /// @brief Pipe file descriptor for death notification.
    int death_pipe_[2]{-1, -1};
    
    /// @brief IProcess instance for native process operations.
    std::unique_ptr<IProcess> process_impl_;
    
    /// @brief SafeProcessMap instance for tracking process termination.
    /// This is a reference to the global SafeProcessMap managed by ProcessGroupManager
    SafeProcessMap* safe_process_map_{nullptr};
};

}  // namespace osal

}  // namespace lcm

}  // namespace internal

}  // namespace score

#endif  // OSAL_NATIVE_EXECUTION_UNIT_HPP_INCLUDED