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

use allocator::{AllocationError, ArenaAllocator, BasicAllocator, MmapMemoryRegion};
use core::alloc::Layout;
use core::ptr::NonNull;

/// Alias to [`sync::ArcIn`] using [`ProtectedMemoryAllocator`].
pub type ProtectedArcIn<T> = sync::ArcIn<T, ProtectedMemoryAllocator>;

/// A memory allocator that provides protected memory regions for health monitoring data structures.
pub struct ProtectedMemoryAllocator(ArenaAllocator<MmapMemoryRegion>);

impl ProtectedMemoryAllocator {
    /// Create a new allocator.
    ///
    /// - `capacity` - allocator capacity in bytes.
    pub fn new(capacity: usize) -> Result<Self, AllocationError> {
        let memory_region = MmapMemoryRegion::new(capacity)?;
        let allocator = ArenaAllocator::new(memory_region)?;
        Ok(Self(allocator))
    }
}

impl BasicAllocator for ProtectedMemoryAllocator {
    fn allocate(&self, layout: Layout) -> Result<NonNull<u8>, AllocationError> {
        self.0.allocate(layout)
    }

    unsafe fn deallocate(&self, ptr: NonNull<u8>, layout: Layout) {
        self.0.deallocate(ptr, layout);
    }
}

impl Default for ProtectedMemoryAllocator {
    fn default() -> Self {
        const DEFAULT_CAPACITY: usize = 1024 * 1024;
        Self::new(DEFAULT_CAPACITY).expect("failed to create allocator with default capacity")
    }
}

impl Clone for ProtectedMemoryAllocator {
    fn clone(&self) -> Self {
        Self(self.0.clone())
    }
}
