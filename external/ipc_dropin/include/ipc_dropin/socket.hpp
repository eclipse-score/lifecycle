#ifndef IPC_DROPIN_SOCKET_HPP_
#define IPC_DROPIN_SOCKET_HPP_

#include <sys/types.h>
#include <string_view>
#include <cstddef>
#include <memory>
#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>

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
        ReturnCode create(const char *name, mode_t mode)
        {
            name_ = name;
            fd_ = shm_open(name, O_CREAT | O_RDWR | O_CLOEXEC, mode);
            if (fd_ < 0)
            {
                if (errno == EACCES)
                {
                    return ReturnCode::kPermissionDenied;
                }
                return ReturnCode::kError;
            }
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
            void *ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
            if (ptr == MAP_FAILED)
            {
                cleanup();
                if (errno == EACCES || errno == EPERM)
                {
                    return ReturnCode::kPermissionDenied;
                }
                return ReturnCode::kError;
            }
            buffer_ = static_cast<RingBuffer<Capacity, ElementSize> *>(ptr);
            buffer_->initialize();
            is_server_ = true;
            return ReturnCode::kOk;
        }

        ReturnCode connect(const char *name) noexcept
        {
            name_ = name;
            fd_ = shm_open(name, O_RDWR | O_CLOEXEC, 0);
            if (fd_ < 0)
            {
                if (errno == EACCES)
                {
                    return ReturnCode::kPermissionDenied;
                }
                return ReturnCode::kError;
            }
            const std::size_t bytes = sizeof(RingBuffer<Capacity, ElementSize>);
            void *ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
            if (ptr == MAP_FAILED)
            {
                cleanup();
                if (errno == EACCES || errno == EPERM)
                {
                    return ReturnCode::kPermissionDenied;
                }
                return ReturnCode::kError;
            }
            buffer_ = static_cast<RingBuffer<Capacity, ElementSize> *>(ptr);

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
                munmap(static_cast<void *>(buffer_), sizeof(RingBuffer<Capacity, ElementSize>));
                buffer_ = nullptr;
            }
            cleanup();
            is_server_ = false;
        }

        std::string_view getName() const noexcept { return name_; }

        bool getOverflowFlag(bool reset = false) noexcept
        {
            if (!buffer_)
            {
                return false;
            }
            return buffer_->getOverflowFlag(reset);
        }

        template <class Payload, typename... Args>
        ReturnCode trySendEmplace(Args &&...args)
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
            return buffer_->template tryEmplace<Payload>(std::forward<Args>(args)...) ? ReturnCode::kOk : ReturnCode::kError;
        }

        template <class Payload>
        ReturnCode tryPeek(Payload *&elem)
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
        ReturnCode tryReceive(Payload &elem)
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
        ReturnCode trySend(Payload &elem)
        {
            if (!buffer_)
            {
                return ReturnCode::kError;
            }
            return buffer_->tryEnqueue(elem) ? ReturnCode::kOk : ReturnCode::kError;
        }

        Socket() = default;
        ~Socket() noexcept { close(); }
        Socket(const Socket &) = delete;
        Socket &operator=(const Socket &) = delete;
        Socket(Socket &&) noexcept = default;
        Socket &operator=(Socket &&) noexcept = default;

    private:
        void cleanup() noexcept
        {
            if (fd_ >= 0)
            {
                ::close(fd_);
                fd_ = -1;
            }
            if (is_server_ && !name_.empty())
            {
                shm_unlink(name_.c_str());
            }
        }

        bool is_server_{false};
        std::string name_;
        int fd_{-1};
        RingBuffer<Capacity, ElementSize> *buffer_{nullptr};
    };

}

#endif