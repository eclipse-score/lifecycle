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
#ifndef TESTS_UTILS_TEST_HELPER_SHM_COUNTER_HPP
#define TESTS_UTILS_TEST_HELPER_SHM_COUNTER_HPP

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace score::mw::lifecycle::test
{

/// @brief A counter shared between processes via POSIX shared memory.
///        Every process which constructs a ShmCounter with the same name sees
///        the same value.
///        The value is an atomic counter.
/// @warning The shared memory object is never removed by the destructor.
///          The user must call `ShmCounter::unlink(name)`.
class ShmCounter
{
  public:
    using value_type = std::int32_t;

    /// @param name Name of the shared memory object, e.g. "/my_counter".
    ///             Must be null terminated.
    explicit ShmCounter(const std::string_view name)
    {
        // use O_EXCL to know if we are the creator.
        bool creator = true;
        fd_ = ::shm_open(name.data(), O_CREAT | O_EXCL | O_RDWR, 0666);
        if (fd_ < 0 && errno == EEXIST)
        {
            creator = false;
            fd_ = ::shm_open(name.data(), O_RDWR, 0666);
        }
        if (fd_ < 0)
        {
            throw_errno("shm_open", name.data());
        }

        if (creator)
        {
            // // make the object accessible to processes running as other users (umask may have masked it).
            // static_cast<void>(::fchmod(fd_, 0666));
            if (::ftruncate(fd_, static_cast<off_t>(sizeof(std::atomic<value_type>))) != 0)
            {
                const int err = errno;
                static_cast<void>(::close(fd_));
                static_cast<void>(::shm_unlink(name.data()));
                errno = err;
                throw_errno("ftruncate", name.data());
            }
        }
        else
        {
            wait_until_sized();
        }

        void* const addr = ::mmap(nullptr, sizeof(std::atomic<value_type>), PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (addr == MAP_FAILED)
        {
            const int err = errno;
            static_cast<void>(::close(fd_));
            errno = err;
            throw_errno("mmap", name.data());
        }
        value_ = static_cast<std::atomic<value_type>*>(addr);
    }

    ~ShmCounter()
    {
        static_cast<void>(::munmap(value_, sizeof(std::atomic<value_type>)));
        static_cast<void>(::close(fd_));
    }

    ShmCounter(const ShmCounter&) = delete;
    ShmCounter& operator=(const ShmCounter&) = delete;
    ShmCounter(ShmCounter&&) = delete;
    ShmCounter& operator=(ShmCounter&&) = delete;

    /// @brief Resets the counter to 0.
    void reset() noexcept
    {
        value_->store(0);
    }

    /// @brief Current value.
    value_type get() const noexcept
    {
        return value_->load();
    }

    /// @brief Atomically increments and returns the NEW value.
    value_type operator++() noexcept
    {
        return value_->fetch_add(1) + 1;
    }

    /// @brief Atomically decrements and returns the NEW value.
    value_type operator--() noexcept
    {
        return value_->fetch_sub(1) - 1;
    }

    /// @brief Atomically increments the counter only if it currently equals `expected`.
    /// @return true if the counter was incremented, false if it held a different value.
    /// @details Async-signal-safe (lock-free atomic), so it may be used from a signal handler.
    bool increment_if_equals(value_type expected) noexcept
    {
        return value_->compare_exchange_strong(expected, expected + 1);
    }

    /// @brief Removes the shared memory object. Existing mappings stay valid. A missing object is not an error.
    /// @warning Call this from the process which knows all other users are finished.
    static void unlink(std::string_view name)
    {
        const std::string name_str{name};
        if (::shm_unlink(name_str.c_str()) != 0 && errno != ENOENT)
        {
            throw_errno("shm_unlink", name_str.c_str());
        }
    }

  private:
    static_assert(
        std::atomic<value_type>::is_always_lock_free,
        "ShmCounter requires a lock-free atomic to work across processes");

    /// Throws a std::runtime_error describing the failed call and the current errno.
    [[noreturn]] static void throw_errno(const char* what, const char* name)
    {
        throw std::runtime_error(
            std::string{"ShmCounter: "} + what + "(\"" + name + "\") failed: " + std::strerror(errno));
    }

    /// The creator may not have called ftruncate yet; mapping a zero-sized object and touching it
    /// would raise SIGBUS. Wait (bounded) until the object has its final size.
    void wait_until_sized()
    {
        constexpr auto kTimeout = std::chrono::seconds{5};
        const auto deadline = std::chrono::steady_clock::now() + kTimeout;
        struct stat st{};
        while (true)
        {
            if (::fstat(fd_, &st) != 0)
            {
                const int err = errno;
                static_cast<void>(::close(fd_));
                errno = err;
                throw_errno("fstat", "");
            }
            if (st.st_size >= static_cast<off_t>(sizeof(std::atomic<value_type>)))
            {
                return;
            }
            if (std::chrono::steady_clock::now() > deadline)
            {
                static_cast<void>(::close(fd_));
                throw std::runtime_error("ShmCounter: timed out waiting for creator to size the object");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
    }

    int fd_{-1};
    std::atomic<value_type>* value_{nullptr};
};

}  // namespace score::mw::lifecycle::test

#endif  // TESTS_UTILS_TEST_HELPER_SHM_COUNTER_HPP
