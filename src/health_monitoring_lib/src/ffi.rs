use crate::common::ffi::*;
use crate::deadline::ffi::DeadlineMonitorCpp;
use crate::*;

#[no_mangle]
extern "C" fn health_monitor_builder_create() -> FFIHandle {
    let builder = HealthMonitorBuilder::new();
    let handle = Box::into_raw(Box::new(builder));
    handle as FFIHandle
}

#[no_mangle]
extern "C" fn health_monitor_builder_destroy(handle: FFIHandle) {
    assert!(!handle.is_null());
    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_create`
    // and this must be assured on other side of FFI.
    unsafe {
        let _ = Box::from_raw(handle as *mut HealthMonitorBuilder);
    }
}

#[no_mangle]
extern "C" fn health_monitor_builder_add_deadline_monitor(handle: FFIHandle, tag: *const IdentTag, monitor: FFIHandle) {
    assert!(!handle.is_null());
    assert!(!tag.is_null());
    assert!(!monitor.is_null());

    // Safety: We ensure that the pointer is valid. `tag` ptr must be FFI data compatible with IdentTag in Rust
    let tag: IdentTag = unsafe { *tag }; // Copy the IdentTag as this shall be trivially copyable

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `deadline_monitor_builder_create`
    let monitor = unsafe { Box::from_raw(monitor as *mut deadline::DeadlineMonitorBuilder) };

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_create`
    // and this must be assured on other side of FFI.
    let mut health_monitor_builder = FFIBorrowed::new(unsafe { Box::from_raw(handle as *mut HealthMonitorBuilder) });

    health_monitor_builder.add_deadline_monitor(tag, *monitor);
}

#[no_mangle]
extern "C" fn health_monitor_builder_build(handle: FFIHandle) -> FFIHandle {
    assert!(!handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_create`
    // and this must be assured on other side of FFI.
    let health_monitor_builder: Box<HealthMonitorBuilder> =
        unsafe { Box::from_raw(handle as *mut HealthMonitorBuilder) };

    let health_monitor = health_monitor_builder.build();
    let health_monitor_handle = Box::into_raw(Box::new(health_monitor));
    health_monitor_handle as FFIHandle
}

#[no_mangle]
extern "C" fn health_monitor_get_deadline_monitor(handle: FFIHandle, tag: *const IdentTag) -> FFIHandle {
    assert!(!handle.is_null());
    assert!(!tag.is_null());

    // Safety: We ensure that the pointer is valid. `tag` ptr must be FFI data compatible with IdentTag in Rust
    let tag: IdentTag = unsafe { *tag }; // Copy the IdentTag as this shall be trivially copyable

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_create`
    // and this must be assured on other side of FFI.
    let mut health_monitor = FFIBorrowed::new(unsafe { Box::from_raw(handle as *mut HealthMonitor) });

    if let Some(deadline_monitor) = health_monitor.get_deadline_monitor(&tag) {
        let deadline_monitor_handle = Box::into_raw(Box::new(DeadlineMonitorCpp::new(deadline_monitor)));

        deadline_monitor_handle as FFIHandle
    } else {
        core::ptr::null_mut()
    }
}

#[no_mangle]
extern "C" fn health_monitor_destroy(handle: FFIHandle) {
    assert!(!handle.is_null());

    // Safety: We ensure that the pointer is valid. We assume that pointer was created by call to `health_monitor_builder_create`
    // and this must be assured on other side of FFI.
    unsafe {
        let _ = Box::from_raw(handle as *mut HealthMonitor);
    }
}
