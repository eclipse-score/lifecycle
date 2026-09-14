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
//! `//config:enable_arm_mte` Bazel flag. Allocation succeeds on every supported
//! target: hardware protection is applied where it is available, and the
//! provider degrades to plain (unprotected) memory everywhere else.
//!
//! - **Feature disabled** (default): regions are served by the Rust allocator,
//!   zero-initialized and 16-byte aligned, without hardware protection.
//! - **Feature enabled on aarch64 Linux**: MTE-protected regions are provided
//!   if the CPU and the kernel support MTE. Otherwise the provider reports an
//!   error and degrades to plain memory.
//! - **Feature enabled on any other target** (e.g. the x86_64 hosts running the
//!   unit tests and the sanitizer builds): the provider reports an error and
//!   degrades to plain memory.
//!
//! Degrading never happens silently: [`ProtectedMemoryAllocator::is_protection_active`]
//! and [`ProtectedMemoryRegion::is_protected`] report whether hardware
//! protection is in effect, so callers that must not run unprotected can detect
//! the degraded configuration.
//!
//! Known limitations:
//!
//! - MTE provides only 16 distinct tags, so distinct regions may share a tag.
//!   Tag 0 is always excluded, which keeps untagged pointers detectable.
//! - Tags are assigned per 16-byte granule, so the granule that holds the end
//!   of a region is tagged as part of it. An access up to 15 bytes past the
//!   requested size still matches the tag and is not detected; detection only
//!   starts at the next granule boundary.
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
}

/// Allocator providing protected memory regions for health monitoring data
/// structures.
pub struct ProtectedMemoryAllocator {}

impl ProtectedMemoryAllocator {
    /// Allocate a new, zero-initialized memory region of `size` bytes.
    ///
    /// The returned region is at least 16-byte aligned (one MTE granule) and is
    /// released when it is dropped. Missing MTE support does not fail the
    /// allocation, see the module documentation for the degraded behavior.
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
    /// Returns `true` if [`Self::allocate`] provides hardware-protected memory,
    /// i.e. the `mte` feature is enabled and both the CPU and the operating
    /// system support MTE.
    ///
    /// Tag checking is a per-thread kernel setting, so where MTE is available
    /// this call enables it for the calling thread as a side effect. Threads
    /// that access protected regions without ever calling this method or
    /// [`Self::allocate`] run without tag checking.
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
    /// Release function of the backend that provided the region.
    release: fn(NonNull<u8>, usize),
    /// Whether the region is protected by hardware memory tagging.
    protected: bool,
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

    /// Check whether this region is protected by hardware memory tagging.
    pub fn is_protected(&self) -> bool {
        self.protected
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
        (self.release)(self.release_ptr, self.release_size);
    }
}

// SAFETY: The region owns its underlying memory. Moving the region to another
// thread moves the ownership and does not create aliasing. Accesses remain
// guarded by the borrow checker (unique or shared references).
unsafe impl Send for ProtectedMemoryRegion {}

// SAFETY: The region content is plain memory. Shared references allow
// read-only access (`&[u8]`), which is thread-safe.
unsafe impl Sync for ProtectedMemoryRegion {}

// MTE backend: hardware-tagged memory, available on aarch64 Linux only.
#[cfg(all(feature = "mte", target_arch = "aarch64", target_os = "linux"))]
use self::mte_linux as sys;

// Plain backend: unprotected memory. Used when the `mte` feature is disabled
// and on every target that cannot provide MTE at all (e.g. the x86_64 hosts
// running the unit tests and the sanitizer builds).
#[cfg(not(all(feature = "mte", target_arch = "aarch64", target_os = "linux")))]
use self::plain as sys;

/// Plain, unprotected memory.
///
/// Regions are served by the Rust allocator instead of `mmap` to keep the
/// behavior identical on all supported platforms (e.g. Linux and QNX use
/// different `mmap` flag values).
mod plain {
    use super::{error, ProtectedMemoryError, ProtectedMemoryRegion, MTE_GRANULE_SIZE};
    use core::alloc::Layout;
    use core::ptr::NonNull;

    /// Alignment of allocated regions (at least one MTE granule).
    const ALIGNMENT: usize = MTE_GRANULE_SIZE;

    /// Allocate a region of `size` bytes, `size` is greater than zero and does
    /// not exceed `isize::MAX`.
    pub(super) fn allocate(size: usize) -> Result<ProtectedMemoryRegion, ProtectedMemoryError> {
        // Protection was requested at build time but cannot be provided here.
        #[cfg(feature = "mte")]
        error!("MTE memory protection is not available, using unprotected memory.");

        let layout = Layout::from_size_align(size, ALIGNMENT).map_err(|_| {
            error!("Requested protected memory size ({}) is invalid.", size);
            ProtectedMemoryError::InvalidArgument
        })?;

        // SAFETY: `layout` has a non-zero size, zero-sized requests are
        // rejected by `ProtectedMemoryAllocator::allocate`.
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
            release,
            protected: false,
        })
    }

    /// Release memory allocated by [`allocate`].
    pub(super) fn release(ptr: NonNull<u8>, size: usize) {
        // SAFETY: `allocate` accepted `size` and `ALIGNMENT` for this region,
        // so they still describe a valid layout and need no re-validation.
        let layout = unsafe { Layout::from_size_align_unchecked(size, ALIGNMENT) };
        // SAFETY: `ptr` was allocated by `allocate` with exactly `layout` and
        // is released only once, when the owning region is dropped.
        unsafe { std::alloc::dealloc(ptr.as_ptr(), layout) };
    }

    /// Plain memory is never protected by hardware.
    pub(super) fn is_protection_active() -> bool {
        false
    }
}

/// MTE-protected memory for aarch64 Linux.
#[cfg(all(feature = "mte", target_arch = "aarch64", target_os = "linux"))]
mod mte_linux {
    use super::{error, plain, ProtectedMemoryError, ProtectedMemoryRegion, MTE_GRANULE_SIZE};
    use core::ptr::NonNull;

    /// Exclude tag 0 when generating random tags, so that pointers without a
    /// tag never match (`irg` excludes tag `i` if bit `i` of the operand is set).
    const TAG_EXCLUDE_ZERO: u64 = 0x1;

    /// MTE support flag in `AT_HWCAP2` (Linux uapi `arch/arm64/include/uapi/asm/hwcap.h`).
    const HWCAP2_MTE: libc::c_ulong = 1 << 18;

    /// Map pages as MTE-tagged (Linux uapi `arch/arm64/include/uapi/asm/mman.h`).
    const PROT_MTE: libc::c_int = 0x20;

    /// Set the tagged address and tag check configuration of the calling thread
    /// (Linux uapi `include/uapi/linux/prctl.h`).
    const PR_SET_TAGGED_ADDR_CTRL: libc::c_int = 55;

    /// Accept tagged pointers in system calls.
    const PR_TAGGED_ADDR_ENABLE: libc::c_ulong = 0x1;

    /// Report tag check faults synchronously, i.e. at the faulting access.
    const PR_MTE_TCF_SYNC: libc::c_ulong = 1 << 1;

    /// Tags that `irg` is allowed to generate, shifted into the tag field of
    /// the control word. All 16 tags are allowed here; tag 0 is excluded
    /// through the `irg` operand instead, see [`TAG_EXCLUDE_ZERO`].
    const PR_MTE_TAG_MASK: libc::c_ulong = 0xffff << 3;

    /// Allocate a region of `size` bytes, `size` is greater than zero and does
    /// not exceed `isize::MAX`.
    pub(super) fn allocate(size: usize) -> Result<ProtectedMemoryRegion, ProtectedMemoryError> {
        if !is_protection_active() {
            return plain::allocate(size);
        }

        // SAFETY: An anonymous mapping uses neither an address hint nor a file
        // descriptor, all remaining arguments are valid constants.
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
            error!("Failed to map protected memory of size ({}).", size);
            return plain::allocate(size);
        }
        // SAFETY: A successful `mmap` returns a valid, page-aligned pointer.
        let ptr = unsafe { NonNull::new_unchecked(raw_ptr.cast::<u8>()) };

        // Tag every granule of the region with a random, non-zero tag and use a
        // pointer carrying this tag for all accesses. Granules that lie fully
        // beyond the region (the kernel rounds the mapping up to page size)
        // keep the zero tag and act as faulting guard areas.
        //
        // SAFETY: `is_protection_active` confirmed that the CPU implements MTE,
        // and `ptr` refers to the `PROT_MTE` mapping of `size` bytes created
        // above, which is the precondition of both functions.
        let tagged_ptr = unsafe {
            let tagged_ptr = create_random_tag(ptr.as_ptr());
            set_region_tag(tagged_ptr, size);
            tagged_ptr
        };

        Ok(ProtectedMemoryRegion {
            // SAFETY: Tagging only modifies the unused top bits of the address,
            // so the tagged pointer is still non-null.
            access_ptr: unsafe { NonNull::new_unchecked(tagged_ptr) },
            release_ptr: ptr,
            release_size: size,
            size,
            release,
            protected: true,
        })
    }

    /// Release memory allocated by [`allocate`].
    pub(super) fn release(ptr: NonNull<u8>, size: usize) {
        // SAFETY: `ptr` is the untagged mapping base returned by `mmap` and
        // `size` is the length that was passed to `mmap`. The mapping is
        // unmapped only once, when the owning region is dropped.
        unsafe { libc::munmap(ptr.as_ptr().cast(), size) };
    }

    /// Check whether MTE protection can be provided and prepare the calling
    /// thread for tag checking.
    ///
    /// Tagged addressing and the tag check mode are per-thread kernel settings,
    /// so they are applied whenever protection is queried or memory is
    /// allocated.
    pub(super) fn is_protection_active() -> bool {
        is_mte_supported_by_cpu() && enable_tag_checking_for_current_thread()
    }

    /// Check whether the CPU implements MTE.
    fn is_mte_supported_by_cpu() -> bool {
        // SAFETY: `getauxval` has no preconditions, it neither dereferences
        // caller-provided pointers nor reports errors through `errno`.
        unsafe { libc::getauxval(libc::AT_HWCAP2) & HWCAP2_MTE != 0 }
    }

    /// Enable tagged addresses and synchronous tag checks for the calling
    /// thread, returns `false` if the kernel does not support MTE.
    fn enable_tag_checking_for_current_thread() -> bool {
        // `PR_SET_TAGGED_ADDR_CTRL` requires the unused arguments to be zero.
        const ZERO: libc::c_ulong = 0;

        let mode = PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC | PR_MTE_TAG_MASK;
        // SAFETY: `prctl` takes the control word of this option by value and
        // dereferences none of its arguments.
        let result = unsafe { libc::prctl(PR_SET_TAGGED_ADDR_CTRL, mode, ZERO, ZERO, ZERO) };
        result == 0
    }

    /// Return `ptr` with a randomly generated, non-zero logical address tag.
    ///
    /// The MTE intrinsics of `core::arch::aarch64` are nightly-only, so the
    /// `irg` instruction is emitted directly.
    ///
    /// # Safety
    ///
    /// The CPU must implement MTE, see [`is_mte_supported_by_cpu`].
    #[target_feature(enable = "mte")]
    unsafe fn create_random_tag(ptr: *mut u8) -> *mut u8 {
        let tagged_addr: usize;
        // SAFETY: `irg` only derives a new logical address tag from `ptr`. It
        // accesses no memory, keeps the condition flags and must not be marked
        // `pure`, because it returns a different tag on every execution.
        unsafe {
            core::arch::asm!(
                "irg {tagged}, {addr}, {excluded}",
                tagged = lateout(reg) tagged_addr,
                addr = in(reg) ptr,
                excluded = in(reg) TAG_EXCLUDE_ZERO,
                options(nomem, nostack, preserves_flags),
            );
        }
        // Take only the address, which carries the new tag, and keep the
        // provenance of the mapping. A pointer read directly out of the `asm!`
        // block would carry no provenance of the allocation it points into.
        ptr.with_addr(tagged_addr)
    }

    /// Store the tag carried by `tagged_ptr` in every granule of the memory
    /// range `tagged_ptr..tagged_ptr + size`.
    ///
    /// # Safety
    ///
    /// The CPU must implement MTE, see [`is_mte_supported_by_cpu`], and
    /// `tagged_ptr` must refer to a `PROT_MTE` mapping of at least `size`
    /// bytes.
    #[target_feature(enable = "mte")]
    unsafe fn set_region_tag(tagged_ptr: *mut u8, size: usize) {
        // Stepping by granule keeps every offset below `size` and therefore
        // free of overflow, and covers the partially used trailing granule.
        for offset in (0..size).step_by(MTE_GRANULE_SIZE) {
            // SAFETY: `offset` is smaller than `size`, so it stays inside the
            // mapping. Byte offsets preserve the logical address tag.
            let granule = unsafe { tagged_ptr.byte_add(offset) };
            // SAFETY: `stg` writes the allocation tag of the granule addressed
            // by `granule`, which lies inside the `PROT_MTE` mapping.
            unsafe {
                core::arch::asm!(
                    "stg {granule}, [{granule}]",
                    granule = in(reg) granule,
                    options(nostack, preserves_flags),
                );
            }
        }
    }
}

#[cfg(all(test, not(loom)))]
#[score_testing_macros::test_mod_with_log]
mod tests {
    use super::{ProtectedMemoryAllocator, ProtectedMemoryError, MTE_GRANULE_SIZE};

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
        let last_byte = region.len() - 1;
        region.as_mut_slice()[last_byte] = 0x55;
        assert_eq!(region.as_slice()[last_byte], 0x55);
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

    // Allocation must always provide usable memory, also on targets and CPUs
    // without MTE support, and must report honestly whether the returned region
    // is hardware-protected.
    #[test]
    fn region_reports_whether_it_is_protected() {
        let allocator = ProtectedMemoryAllocator {};
        let region = allocator.allocate(64).expect("allocation should succeed");

        assert_eq!(region.is_protected(), allocator.is_protection_active());
    }

    // Protection must never be active when the MTE feature is disabled.
    #[cfg(not(feature = "mte"))]
    #[test]
    fn protection_is_inactive_by_default() {
        let allocator = ProtectedMemoryAllocator {};
        assert!(!allocator.is_protection_active());

        let region = allocator.allocate(64).expect("allocation should succeed");
        assert!(!region.is_protected());
    }

    // The MTE feature may be enabled on targets that cannot provide MTE (e.g.
    // the x86_64 hosts running the unit tests and the sanitizer builds). The
    // provider then falls back to unprotected memory instead of failing.
    #[cfg(all(feature = "mte", not(all(target_arch = "aarch64", target_os = "linux"))))]
    #[test]
    fn allocate_falls_back_if_mte_is_unsupported() {
        let allocator = ProtectedMemoryAllocator {};
        assert!(!allocator.is_protection_active());

        let region = allocator.allocate(64).expect("allocation should succeed");
        assert!(!region.is_protected());
    }
}
