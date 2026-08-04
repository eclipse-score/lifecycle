#include "score/mw/launch_manager/osal/native_execution_unit.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <cstdint>

namespace score {

namespace lcm {

namespace internal {

namespace osal {

NativeExecutionUnit::NativeExecutionUnit() {
    // Create a pipe for death notifications
    if (pipe(death_pipe_) != 0) {
        LM_LOG_ERROR() << "Failed to create death notification pipe: " << strerror(errno);
        death_pipe_[0] = -1;
        death_pipe_[1] = -1;
    }
    
    // Set the read end to be non-blocking
    if (death_pipe_[0] != -1) {
        int flags = fcntl(death_pipe_[0], F_GETFL, 0);
        if (flags != -1) {
            fcntl(death_pipe_[0], F_SETFL, flags | O_NONBLOCK);
        }
    }
    
    process_impl_ = std::make_unique<IProcess>();
}

NativeExecutionUnit::~NativeExecutionUnit() {
    if (death_pipe_[0] != -1) {
        close(death_pipe_[0]);
    }
    if (death_pipe_[1] != -1) {
        close(death_pipe_[1]);
    }
}

std::uint64_t NativeExecutionUnit::generateToken() {
    return token_counter_.fetch_add(1, std::memory_order_relaxed);
}

OsalReturnType NativeExecutionUnit::start(const ExecutionSpec& spec,
                                          ExecutionHandle& out,
                                          IpcCommsP* comms) {
    if (!process_impl_) {
        LM_LOG_ERROR() << "Process implementation not available";
        return OsalReturnType::kFail;
    }
    
    // Convert ExecutionSpec to OsalConfig
    osal::OsalConfig config{};
    config.short_name_ = spec.short_name_;
    config.executable_path_ = spec.executable_path_;
    config.argv_ = spec.argv_;
    config.envp_ = spec.envp_;
    config.uid_ = spec.uid_;
    config.gid_ = spec.gid_;
    config.supplementary_gids_ = spec.supplementary_gids_;
    config.comms_type_ = spec.comms_type_;
    config.cpu_mask_ = spec.cpu_mask_;
    config.scheduling_policy_ = spec.scheduling_policy_;
    config.scheduling_priority_ = spec.scheduling_priority_;
    
    // Generate a unique token for this process
    std::uint64_t token = generateToken();
    
    // Create the process using IProcess interface
    ProcessID pid = 0;
    OsalReturnType result = process_impl_->startProcess(&pid, comms, &config);
    
    if (result == OsalReturnType::kSuccess) {
        // Store mapping from token to PID and status
        std::lock_guard<std::mutex> lock(process_map_mutex_);
        process_map_[token] = {pid, 0};
        
        // Set the output handle
        out = ExecutionHandle(token);
        
        // Register with SafeProcessMap for termination notification
        if (safe_process_map_) {
            auto insert_result = safe_process_map_->insertIfNotTerminated(pid, this);
            if (insert_result == SafeProcessMap::SafeProcessMapReturnType::kYield) {
                // Process has already terminated before we could register it
                // This is a race condition - mark as terminated immediately
                std::lock_guard<std::mutex> lock(process_map_mutex_);
                process_map_[token].status = -1;  // Mark as terminated with error
            }
        } else {
            LM_LOG_WARN() << "SafeProcessMap not available, process termination will not be tracked";
        }
    }
    
    return result;
}

OsalReturnType NativeExecutionUnit::requestTermination(const ExecutionHandle& handle) {
    std::lock_guard<std::mutex> lock(process_map_mutex_);
    auto it = process_map_.find(handle.token());
    if (it == process_map_.end()) {
        LM_LOG_ERROR() << "Process with token " << handle.token() << " not found";
        return OsalReturnType::kFail;
    }
    
    ProcessID pid = it->second.pid;
    auto result = process_impl_->requestTermination(pid);
    
    // If SafeProcessMap is available, remove the entry to prevent duplicate notifications
    if (safe_process_map_) {
        safe_process_map_->findTerminated(pid, it->second.status);
    }
    
    return result;
}

OsalReturnType NativeExecutionUnit::forceTermination(const ExecutionHandle& handle) {
    std::lock_guard<std::mutex> lock(process_map_mutex_);
    auto it = process_map_.find(handle.token());
    if (it == process_map_.end()) {
        LM_LOG_ERROR() << "Process with token " << handle.token() << " not found";
        return OsalReturnType::kFail;
    }
    
    ProcessID pid = it->second.pid;
    auto result = process_impl_->forceTermination(pid);
    
    // If SafeProcessMap is available, remove the entry to prevent duplicate notifications
    if (safe_process_map_) {
        safe_process_map_->findTerminated(pid, it->second.status);
    }
    
    return result;
}

int NativeExecutionUnit::pollable_fd() const {
    if (death_pipe_[0] != -1) {
        return death_pipe_[0];
    }
    // Fallback to a valid fd even if pipe creation failed
    return -1;
}

bool NativeExecutionUnit::drain(std::vector<DeathEvent>& out) {
    // For native processes, we rely on the OsHandler's waitpid mechanism
    // This method would be called when the pipe becomes readable.
    
    // Read from the pipe to clear the notification
    char buffer[64];
    ssize_t bytes_read = read(death_pipe_[0], buffer, sizeof(buffer));
    if (bytes_read > 0) {
        // We received a death notification via pipe
        // The actual process information is already handled in terminated()
        return true;
    }
    
    return false;
}

void NativeExecutionUnit::terminated(int32_t process_status) {
    // Find the process that terminated
    std::lock_guard<std::mutex> lock(process_map_mutex_);
    for (auto& entry : process_map_) {
        if (entry.second.status == 0) {  // Unmarked process
            entry.second.status = process_status;
            
            // Make the pipe writable to signal death notification
            if (death_pipe_[1] != -1) {
                const char sig = '1';
                ssize_t bytes_written = write(death_pipe_[1], &sig, sizeof(sig));
                if (bytes_written < 0) {
                    LM_LOG_WARN() << "Failed to write to death notification pipe: " << strerror(errno);
                }
            }
            
            // Remove from SafeProcessMap to prevent duplicate notifications
            if (safe_process_map_) {
                safe_process_map_->findTerminated(entry.second.pid, process_status);
            }
            
            LM_LOG_DEBUG() << "Native process " << entry.second.pid << " terminated with status " << process_status;
            return;
        }
    }
}

// Static method to set the SafeProcessMap instance
void NativeExecutionUnit::setSafeProcessMap(SafeProcessMap* map) {
    // This would need to be a static method or the unit should be constructed with the map
    // For now, we'll leave this as a placeholder
}

}  // namespace osal

}  // namespace lcm

}  // namespace internal

}  // namespace score