// *******************************************************************************
// Copyright (c) 2026 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

//! Protected memory provider for health monitoring data.
//!
//! The provider allocates memory regions that are protected against unintended
//! access using the ARM Memory Tagging Extension (MTE, ARMv8.5).
//! A region is mapped with the `PROT_MTE` flag and every 16-byte MTE granule is
//! assigned a randomly generated, non-zero tag. The returned
//! [`ProtectedMemoryRegion`] carries this tag in its pointers, so valid accesses
//! through the region API always match the memory tags. Any stray pointer into
//! the region (e.g. a corrupted or out-of-bounds pointer) carries a different
//! tag and triggers a tag check fault (synchronous mode) instead of silently
//! corrupting health monitoring data.
//!
//! The provider is gated behind the `mte` feature, which is controlled by the
//! `//config:enable_arm_mte` Bazel flag:
//!
//! - **Feature disabled** (default): regions are allocated from plain,
//!   unprotected (but still zero-initialized and 16-byte aligned) memory.
//! - **Feature enabled on aarch64 Linux**: MTE-protected regions are provided.
//!   The ARM CPU must support MTE, otherwise allocation fails at runtime with
//!   [`ProtectedMemoryError::MteNotSupported`].
//! - **Feature enabled on other targets**: allocation fails at runtime with
//!   [`ProtectedMemoryError::MteNotSupported`], code still compiles cleanly.
//!
//! Known limitations:
//!
//! - MTE provides only 16 distinct tags, so distinct regions may share a tag.
//!   Tag 0 is always excluded, which keeps untagged pointers detectable.
//! - Tag check configuration (`PR_MTE_TCF_SYNC`) and tagged address support
//!   (`PR_TAGGED_ADDR_ENABLE`) are per-thread kernel settings. They are enabled
//!   for the calling thread on each allocation. Threads that only access
//!   (but do not allocate) protected regions inherit no tag checking and need
//!   to be covered when the provider gets integrated into the monitors.

// The provider is not yet wired into the monitors, see
// https://github.com/eclipse-score/lifecycle/issues/119.
#![allow(dead_code)]

use crate::log::{error, ScoreDebug};
use core::ptr::NonNull;

/// MTE granule (tagging granularity) size in bytes.
const MTE_GRANULE_SIZE: usize = 16;

/// Errors of the protected memory provider.
#[derive(PartialEq, Eq, Debug, ScoreDebug)]
pub enum ProtectedMemoryError {
    /// Provided argument is invalid.
    InvalidArgument,
    /// System was unable to provide the requested memory.
    OutOfMemory,
    /// Protected memory is not supported by the hardware or the operating system.
    MteNotSupported,
}

/// Allocator providing protected memory regions for health monitoring data
/// structures.
pub struct ProtectedMemoryAllocator {}

impl ProtectedMemoryAllocator {
    /// Allocate a new, zero-initialized memory region of `size` bytes.
    ///
    /// The returned region is at least 16-byte aligned (one MTE granule) and is
    /// released when it is dropped.
    ///
    /// - `size` - size of the region in bytes, must be greater than zero.
    pub fn allocate(&self, size: usize) -> Result<ProtectedMemoryRegion, ProtectedMemoryError> {
        if size == 0 || size > isize::MAX as usize {
            error!("Requested protected memory size ({}) is invalid.", size);
            return Err(ProtectedMemoryError::InvalidArgument);
        }

        sys::allocate(size)
    }

    /// Check whether memory protection is active on this platform.
    ///
    /// Returns `true` if [`Self::allocate`] provides hardware-protected memory
    /// (MTE available and enabled via the `mte` feature).
    pub fn is_protection_active(&self) -> bool {
        sys::is_protection_active()
    }
}

/// Region of protected memory allocated by [`ProtectedMemoryAllocator`].
///
/// The region is released when it is dropped.
#[derive(Debug)]
pub struct ProtectedMemoryRegion {
    /// Pointer used to access the region (carries the MTE tag when protection is active).
    access_ptr: NonNull<u8>,
    /// Pointer used to release the underlying memory (never MTE-tagged).
    release_ptr: NonNull<u8>,
    /// Size passed to the release function.
    release_size: usize,
    /// Size of the region visible to the user.
    size: usize,
}

impl ProtectedMemoryRegion {
    /// Size of the region in bytes.
    pub fn len(&self) -> usize {
        self.size
    }

    /// Check whether the region is empty.
    ///
    /// Regions are never empty, [`ProtectedMemoryAllocator::allocate`] rejects
    /// zero-sized requests.
    pub fn is_empty(&self) -> bool {
        false
    }

    /// Pointer to the first byte of the region.
    pub fn as_ptr(&self) -> *const u8 {
        self.access_ptr.as_ptr()
    }

    /// Mutable pointer to the first byte of the region.
    pub fn as_mut_ptr(&mut self) -> *mut u8 {
        self.access_ptr.as_ptr()
    }

    /// Region content.
    pub fn as_slice(&self) -> &[u8] {
        // SAFETY: `access_ptr` is valid for reads of `size` bytes for the
        // lifetime of the region and `size` does not exceed `isize::MAX`
        // (validated during allocation). The returned borrow does not outlive
        // the region.
        unsafe { core::slice::from_raw_parts(self.access_ptr.as_ptr(), self.size) }
    }

    /// Mutable region content.
    pub fn as_mut_slice(&mut self) -> &mut [u8] {
        // SAFETY: `access_ptr` is valid for reads and writes of `size` bytes
        // for the lifetime of the region and `size` does not exceed
        // `isize::MAX` (validated during allocation). The returned borrow has
        // the lifetime of the unique region reference.
        unsafe { core::slice::from_raw_parts_mut(self.access_ptr.as_ptr(), self.size) }
    }
}

impl Drop for ProtectedMemoryRegion {
    fn drop(&mut self) {
        sys::release(self.release_ptr, self.release_size);
    }
}

// SAFETY: The region owns its underlying memory. Moving the region to another
// thread moves the ownership and does not create aliasing. Accesses remain
// guarded by the borrow checker (unique or shared references).
unsafe impl Send for ProtectedMemoryRegion {}

// SAFETY: The region content is plain memory. Shared references allow
// read-only access (`&[u8]`), which is thread-safe.
unsafe impl Sync for ProtectedMemoryRegion {}

// Fallback backend: plain, unprotected memory.
// Regions are served by the Rust allocator instead of `mmap` to keep the
// behavior identical on all supported platforms (e.g. Linux and QNX use
// different `mmap` flag values).
#[cfg(not(feature = "mte"))]
use self::fallback as sys;

// MTE backend: hardware-tagged memory for aarch64 Linux.
#[cfg(all(feature = "mte", target_arch = "aarch64", target_os = "linux"))]
use self::mte_linux as sys;

// Backend for targets where MTE protection is requested but unsupported.
#[cfg(all(feature = "mte", not(all(target_arch = "aarch64", target_os = "linux"))))]
use self::unsupported as sys;

#[cfg(not(feature = "mte"))]
mod fallback {
    use super::{error, MTE_GRANULE_SIZE, ProtectedMemoryError, ProtectedMemoryRegion};
    use core::alloc::Layout;
    use core::ptr::NonNull;

    /// Alignment of allocated regions (at least one MTE granule).
    const ALIGNMENT: usize = MTE_GRANULE_SIZE;

    /// Allocate a region of `size` bytes, `size` is greater than zero and does
    /// not exceed `isize::MAX`.
    pub(super) fn allocate(size: usize) -> Result<ProtectedMemoryRegion, ProtectedMemoryError> {
        let layout = Layout::from_size_align(size, ALIGNMENT).map_err(|_| {
            error!("Requested protected memory size ({}) is invalid.", size);
            ProtectedMemoryError::InvalidArgument
        })?;

        // SAFETY: layout has non-zero size (validated by the caller).
        let raw_ptr = unsafe { std::alloc::alloc_zeroed(layout) };
        let ptr = NonNull::new(raw_ptr).ok_or_else(|| {
            error!("Failed to allocate protected memory of size ({}).", size);
            ProtectedMemoryError::OutOfMemory
        })?;

        Ok(ProtectedMemoryRegion {
            access_ptr: ptr,
            release_ptr: ptr,
            release_size: size,
            size,
        })
    }

    /// Release memory allocated by [`allocate`].
    pub(super) fn release(ptr: NonNull<u8>, size: usize) {
        // SAFETY: `ptr` was allocated by `allocate` with the same layout
        // parameters.
        unsafe { std::alloc::dealloc(ptr.as_ptr(), Layout::from_size_align_unchecked(size, ALIGNMENT)) };
    }

    /// Hardware protection is not requested in this configuration.
    pub(super) fn is_protection_active() -> bool {
        false
    }
}

#[cfg(all(feature = "mte", target_arch = "aarch64", target_os = "linux"))]
mod mte_linux {
    use super::{error, MTE_GRANULE_SIZE, ProtectedMemoryError, ProtectedMemoryRegion};
    use core::ptr::NonNull;

    /// Exclude tag 0 when generating random tags, so that pointers without a
    /// tag never match (Linux uapi: tag i is excluded if bit i is set).
    const TAG_EXCLUDE_ZERO: u64 = 0x1;

    /// MTE support flag in `AT_HWCAP2` (Linux uapi arch/arm64/include/uapi/asm/hwcap.h).
    const HWCAP2_MTE: libc::c_ulong = 1 << 18;

    /// Enable tagged memory pages (Linux uapi arch/arm64/include/uapi/asm/mman.h).
    const PROT_MTE: libc::c_int = 0x20;

    /// Enable tagged addresses and MTE synchronous tag checks for the calling
    /// thread (Linux uapi include/uapi/linux/prctl.h).
    const PR_SET_TAGGED_ADDR_CTRL: libc::c_int = 56;
    const PR_TAGGED_ADDR_ENABLE: libc::c_ulong = 1 << 62;
    const PR_MTE_TCF_SYNC: libc::c_ulong = 1 << 1;

    /// Allocate a region of `size` bytes, `size` is greater than zero and does
    /// not exceed `isize::MAX`.
    pub(super) fn allocate(size: usize) -> Result<ProtectedMemoryRegion, ProtectedMemoryError> {
        if !is_protection_active() {
            error!("MTE memory protection is requested but not supported by the hardware.");
            return Err(ProtectedMemoryError::MteNotSupported);
        }

        if enable_tag_checking_for_current_thread().is_err() {
            error!("MTE memory protection is requested but the operating system does not support it.");
            return Err(ProtectedMemoryError::MteNotSupported);
        }

        // SAFETY: all arguments are valid constants, anonymous mapping does
        // not use a file descriptor.
        let raw_ptr = unsafe {
            libc::mmap(
                core::ptr::null_mut(),
                size,
                libc::PROT_READ | libc::PROT_WRITE | PROT_MTE,
                libc::MAP_PRIVATE | libc::MAP_ANONYMOUS,
                -1,
                0,
            )
        };
        if raw_ptr == libc::MAP_FAILED {
            error!("Failed to allocate protected memory of size ({}).", size);
            return Err(ProtectedMemoryError::OutOfMemory);
        }
        // SAFETY: successful `mmap` returns a valid, page-aligned pointer.
        let ptr = unsafe { NonNull::new_unchecked(raw_ptr.cast::<u8>()) };

        // Tag every granule of the region with a random, non-zero tag and use
        // a pointer carrying this tag for accesses. Granules beyond `size`
        // (the kernel rounds up to page size) keep the zero tag and act as
        // faulting guard areas.
        //
        // SAFETY: MTE availability is checked above.
        let tagged_ptr = unsafe { create_random_tag(ptr.as_ptr()) };
        // SAFETY: MTE availability is checked above.
        unsafe { set_region_tag(tagged_ptr, size) };

        Ok(ProtectedMemoryRegion {
            // SAFETY: tagging only modifies the upper bits, the pointer
            // remains valid and non-null.
            access_ptr: unsafe { NonNull::new_unchecked(tagged_ptr) },
            release_ptr: ptr,
            release_size: size,
            size,
        })
    }

    /// Release memory allocated by [`allocate`].
    pub(super) fn release(ptr: NonNull<u8>, size: usize) {
        // SAFETY: `ptr` is the untagged mapping base returned by `mmap` and
        // `size` the length passed to `mmap`.
        unsafe { libc::munmap(ptr.as_ptr().cast(), size) };
    }

    /// Check whether the platform supports MTE.
    pub(super) fn is_protection_active() -> bool {
        // SAFETY: `getauxval` has no preconditions.
        unsafe { libc::getauxval(libc::AT_HWCAP2) & HWCAP2_MTE != 0 }
    }

    /// Enable MTE synchronous tag checks and tagged addresses for the calling
    /// thread.
    fn enable_tag_checking_for_current_thread() -> Result<(), ProtectedMemoryError> {
        // SAFETY: prctl with `PR_SET_TAGGED_ADDR_CTRL` has no pointer
        // arguments.
        let result = unsafe {
            libc::prctl(
                PR_SET_TAGGED_ADDR_CTRL,
                PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC,
                0u64,
                0u64,
                0u64,
            )
        };
        if result != 0 {
            return Err(ProtectedMemoryError::MteNotSupported);
        }
        Ok(())
    }

    /// Create a pointer to `ptr` with a random, non-zero tag.
    #[target_feature(enable = "mte")]
    unsafe fn create_random_tag(ptr: *mut u8) -> *mut u8 {
        // SAFETY: caller guarantees MTE is available on the platform.
        unsafe { core::arch::aarch64::__arm_mte_create_random_tag(ptr, TAG_EXCLUDE_ZERO) }
    }

    /// Store the tag carried by `tagged_ptr` in every granule of the memory
    /// range `tagged_ptr..tagged_ptr+size`.
    #[target_feature(enable = "mte")]
    unsafe fn set_region_tag(tagged_ptr: *mut u8, size: usize) {
        let num_granules = size.div_ceil(MTE_GRANULE_SIZE);
        for i in 0..num_granules {
            // SAFETY: pointer arithmetic stays within the mapped region and
            // preserves the tag. `__arm_mte_set_tag` stores the tag carried by
            // the pointer at the granule the pointer refers to.
            unsafe {
                core::arch::aarch64::__arm_mte_set_tag(tagged_ptr.byte_add(i * MTE_GRANULE_SIZE));
            }
        }
    }
}

#[cfg(all(feature = "mte", not(all(target_arch = "aarch64", target_os = "linux"))))]
mod unsupported {
    use super::{error, ProtectedMemoryError, ProtectedMemoryRegion};
    use core::ptr::NonNull;

    /// MTE protection is requested but this target does not support it.
    pub(super) fn allocate(size: usize) -> Result<ProtectedMemoryRegion, ProtectedMemoryError> {
        error!(
            "MTE memory protection is requested but not supported on this target (size: {}).",
            size
        );
        Err(ProtectedMemoryError::MteNotSupported)
    }

    /// Nothing was allocated, nothing to release.
    pub(super) fn release(_ptr: NonNull<u8>, _size: usize) {}

    /// Hardware protection is not available in this configuration.
    pub(super) fn is_protection_active() -> bool {
        false
    }
}

#[cfg(all(test, not(loom)))]
#[score_testing_macros::test_mod_with_log]
mod tests {
    use super::{MTE_GRANULE_SIZE, ProtectedMemoryAllocator, ProtectedMemoryError};

    #[test]
    fn allocate_returns_requested_size() {
        let allocator = ProtectedMemoryAllocator {};
        for &size in &[1usize, MTE_GRANULE_SIZE, 64, 4096, 100_000] {
            let region = allocator.allocate(size).expect("allocation should succeed");
            assert_eq!(region.len(), size);
        }
    }

    #[test]
    fn allocate_rejects_zero_size() {
        let allocator = ProtectedMemoryAllocator {};
        assert!(matches!(
            allocator.allocate(0),
            Err(ProtectedMemoryError::InvalidArgument)
        ));
    }

    #[test]
    fn region_is_granule_aligned() {
        let allocator = ProtectedMemoryAllocator {};
        for _ in 0..16 {
            let region = allocator.allocate(1).expect("allocation should succeed");
            assert_eq!(region.as_ptr() as usize % MTE_GRANULE_SIZE, 0);
        }
    }

    #[test]
    fn region_is_zero_initialized() {
        let allocator = ProtectedMemoryAllocator {};
        let region = allocator.allocate(4096).expect("allocation should succeed");
        assert!(region.as_slice().iter().all(|&byte| byte == 0));
    }

    #[test]
    fn region_supports_read_write() {
        let allocator = ProtectedMemoryAllocator {};
        let mut region = allocator.allocate(1024).expect("allocation should succeed");

        region.as_mut_slice().fill(0xAA);
        assert!(region.as_slice().iter().all(|&byte| byte == 0xAA));

        // Boundary access at the last byte.
        region.as_mut_slice()[region.len() - 1] = 0x55;
        assert_eq!(region.as_slice()[region.len() - 1], 0x55);
    }

    #[test]
    fn multiple_regions_are_independent() {
        let allocator = ProtectedMemoryAllocator {};
        let mut region_1 = allocator.allocate(64).expect("allocation should succeed");
        let mut region_2 = allocator.allocate(64).expect("allocation should succeed");

        region_1.as_mut_slice().fill(0x11);
        region_2.as_mut_slice().fill(0x22);

        assert!(region_1.as_slice().iter().all(|&byte| byte == 0x11));
        assert!(region_2.as_slice().iter().all(|&byte| byte == 0x22));
    }

    #[test]
    fn region_is_released_on_drop() {
        let allocator = ProtectedMemoryAllocator {};
        for _ in 0..100 {
            let mut region = allocator.allocate(64 * 1024).expect("allocation should succeed");
            region.as_mut_slice()[0] = 1;
        }
        // Allocator still works after repeated allocations and releases.
        let region = allocator.allocate(16).expect("allocation should succeed");
        assert_eq!(region.len(), 16);
    }

    // Protection must never be active when the MTE feature is disabled.
    #[cfg(not(feature = "mte"))]
    #[test]
    fn protection_is_inactive_by_default() {
        let allocator = ProtectedMemoryAllocator {};
        assert!(!allocator.is_protection_active());
        assert!(allocator.allocate(16).is_ok());
    }

    // When MTE is requested on a target without MTE support (e.g. x86_64 CI
    // hosts), allocation must fail at runtime instead of providing
    // unprotected memory.
    #[cfg(all(feature = "mte", not(all(target_arch = "aarch64", target_os = "linux"))))]
    #[test]
    fn allocate_fails_if_mte_is_unsupported() {
        let allocator = ProtectedMemoryAllocator {};
        assert!(matches!(
            allocator.allocate(16),
            Err(ProtectedMemoryError::MteNotSupported)
        ));
        assert!(!allocator.is_protection_active());
    }
}
