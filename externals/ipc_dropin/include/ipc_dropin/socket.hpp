#ifndef IPC_DROPIN_SOCKET_HPP_
#define IPC_DROPIN_SOCKET_HPP_

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

#include "ringbuffer.hpp"

namespace ipc_dropin
{
enum class ReturnCode
{
    kOk,
    kError,
    kQueueEmpty,
    kPermissionDenied,
    kQueueStateCorrupt,
};

template <std::size_t ElementSize, std::size_t Capacity>
class Socket
{
  public:
    ReturnCode create(const char* name, mode_t mode)
    {
        name_ = name;
        if (name_.size() > 0 && name_[0] != '/')
        {
            name_ = "/" + name_;
        }

        // shm_open will reopen shmem files if they already exist
        // need to perform this check explicitly beforehand
        if (exists(name_, mode))
        {
            // shm was likely not cleaned up from last run (e.g. due to a crash)
            if (shm_unlink(name_.c_str()) != 0)
            {
                if (errno == EACCES)
                {
                    return ReturnCode::kPermissionDenied;
                }
                return ReturnCode::kError;
            }
        }

        fd_ = shm_open(name_.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, mode);
        if (fd_ < 0)
        {
            if (errno == EACCES)
            {
                return ReturnCode::kPermissionDenied;
            }
            return ReturnCode::kError;
        }
        owns_shared_memory_ = true;
        const std::size_t bytes = sizeof(RingBuffer<Capacity, ElementSize>);
        if (ftruncate(fd_, bytes) != 0)
        {
            cleanup();
            if (errno == EACCES || errno == EPERM)
            {
                return ReturnCode::kPermissionDenied;
            }
            return ReturnCode::kError;
        }
        void* ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (ptr == MAP_FAILED)
        {
            cleanup();
            if (errno == EACCES || errno == EPERM)
            {
                return ReturnCode::kPermissionDenied;
            }
            return ReturnCode::kError;
        }
        buffer_ = static_cast<RingBuffer<Capacity, ElementSize>*>(ptr);
        buffer_->initialize();
        return ReturnCode::kOk;
    }

    ReturnCode connect(const char* name) noexcept
    {
        name_ = name;
        if (name_.size() > 0 && name_[0] != '/')
        {
            name_ = "/" + name_;
        }

        fd_ = shm_open(name_.c_str(), O_RDWR | O_CLOEXEC, 0);
        if (fd_ < 0)
        {
            if (errno == EACCES)
            {
                return ReturnCode::kPermissionDenied;
            }
            return ReturnCode::kError;
        }
        const std::size_t bytes = sizeof(RingBuffer<Capacity, ElementSize>);
        void* ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (ptr == MAP_FAILED)
        {
            cleanup();
            if (errno == EACCES || errno == EPERM)
            {
                return ReturnCode::kPermissionDenied;
            }
            return ReturnCode::kError;
        }
        buffer_ = static_cast<RingBuffer<Capacity, ElementSize>*>(ptr);

        if (!buffer_->isInitialized())
        {
            return ReturnCode::kError;
        }
        return ReturnCode::kOk;
    }

    void close() noexcept
    {
        if (buffer_)
        {
            munmap(static_cast<void*>(buffer_), sizeof(RingBuffer<Capacity, ElementSize>));
            buffer_ = nullptr;
        }
        cleanup();
    }

    std::string_view getName() const noexcept
    {
        return name_;
    }

    bool getOverflowFlag(bool reset = false) noexcept
    {
        if (!buffer_)
        {
            return false;
        }
        return buffer_->getOverflowFlag(reset);
    }

    template <class Payload, typename... Args>
    ReturnCode trySendEmplace(Args&&... args)
    {
        static_assert(std::is_trivially_copyable<Payload>::value, "Payload must be trivially copyable");
        if (!buffer_)
        {
            return ReturnCode::kError;
        }
        if (sizeof(Payload) > ElementSize)
        {
            return ReturnCode::kError;
        }
        return buffer_->template tryEmplace<Payload>(std::forward<Args>(args)...) ? ReturnCode::kOk
                                                                                  : ReturnCode::kError;
    }

    template <class Payload>
    ReturnCode tryPeek(Payload*& elem)
    {
        if (!buffer_)
        {
            return ReturnCode::kError;
        }
        if (buffer_->empty())
        {
            return ReturnCode::kQueueEmpty;
        }
        if (buffer_->template tryPeek(elem))
        {
            return ReturnCode::kOk;
        }
        return ReturnCode::kError;
    }

    ReturnCode tryPop()
    {
        if (!buffer_)
        {
            return ReturnCode::kError;
        }
        return buffer_->tryPop() ? ReturnCode::kOk : ReturnCode::kError;
    }

    template <class Payload>
    ReturnCode tryReceive(Payload& elem)
    {
        if (!buffer_)
        {
            return ReturnCode::kQueueStateCorrupt;
        }
        if (buffer_->empty())
        {
            return ReturnCode::kQueueEmpty;
        }
        return buffer_->tryDequeue(elem) ? ReturnCode::kOk : ReturnCode::kError;
    }

    template <class Payload>
    ReturnCode trySend(Payload& elem)
    {
        if (!buffer_)
        {
            return ReturnCode::kError;
        }
        return buffer_->tryEnqueue(elem) ? ReturnCode::kOk : ReturnCode::kError;
    }

    Socket() = default;
    ~Socket() noexcept
    {
        close();
    }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&&) noexcept = default;
    Socket& operator=(Socket&&) noexcept = default;

  private:
    void cleanup() noexcept
    {
        if (fd_ >= 0)
        {
            ::close(fd_);
            fd_ = -1;
        }
        if (owns_shared_memory_ && !name_.empty())
        {
            shm_unlink(name_.c_str());
            owns_shared_memory_ = false;
        }
    }

    bool owns_shared_memory_{false};
    std::string name_;
    int fd_{-1};
    RingBuffer<Capacity, ElementSize>* buffer_{nullptr};

    /// @brief Check if a shared memory file with given path already exists
    /// @param [in] f_name The name of shared memory file
    /// @param [in] f_mode The permission bits of shared memory file
    /// @return True if shmem file already exists, else false
    static bool exists(const std::string& f_name, mode_t f_mode) noexcept(false)
    {
        // Try opening the shmem without creating it to check if shmem files already exist
        auto fd = shm_open(f_name.c_str(), O_RDONLY, f_mode);
        if (fd >= 0)
        {
            ::close(fd);
            return true;
        }
        return false;
    }
};

}  // namespace ipc_dropin

#endif
