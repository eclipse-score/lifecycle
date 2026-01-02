// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0

use crate::common::{Error, Status, Tag, DurationRange};
use iceoryx2_bb_lock_free::mpmc::container::*;
use std::{
    collections::HashMap, sync::{Arc, Mutex, atomic::Ordering, atomic::AtomicU32}, time::{Duration, Instant}
};

struct DeadlineTemplate {
    range: DurationRange,
}

/// A DeadlineMonitorBuilder is used to create a [`DeadlineMonitor`].
pub struct DeadlineMonitorBuilder {
    templates: HashMap<Tag, DeadlineTemplate>,
}

impl DeadlineMonitorBuilder {
    /// Creates the builder.
    pub fn new() -> Self {
        DeadlineMonitorBuilder { templates: HashMap::new() }
    }

    /// Adds a deadline that can then be created via [`DeadlineMonitor::get_deadline`] and
    /// [`DeadlineMonitor::get_deadline_guard`].
    pub fn add_deadline(&mut self, tag: Tag, range: DurationRange) -> &mut Self {
        self.templates.insert(tag, DeadlineTemplate { range });

        self
    }

    /// Builds the [`DeadlineMonitor`].
    pub fn build(self) -> Result<DeadlineMonitor, Error> {
        Ok(DeadlineMonitor {
            inner: Arc::new(Inner {
                container: FixedSizeContainer::new(),
                status: AtomicU32::new(Status::Running.into()),
                timer: Mutex::new(Timer::new()),
                templates: self.templates,
            }),
        })
    }
}

/// The DeadlineMonitor is used to create [`Deadline`] and [`DeadlineGuard`] objects that can be
/// used to validate that given code executes within a set [`DurationRange`]. The DeadlineMonitor
/// is designed to be periodically evaluated by a [`HealthMonitor`].
#[derive(Clone)]
pub struct DeadlineMonitor {
    inner: Arc<Inner>,
}

impl DeadlineMonitor {
    /// Creates a [`Deadline`] added by [`DeadlineMonitorBuilder::add_deadline`].
    pub fn get_deadline(&self, tag: Tag) -> Result<Deadline, Error> {
        if let Some(template) = self.inner.templates.get(&tag) {
            Ok(Deadline {
                inner: Arc::clone(&self.inner),
                range: template.range,
                handle: None,
                min_elapsed: Duration::ZERO,
                max_elapsed: Duration::ZERO,
            })
        } else {
            Err(Error::DoesNotExist)
        }
    }

    /// Creates a [`DeadlineGuard`] added by [`DeadlineMonitorBuilder::add_deadline`].
    pub fn get_deadline_guard(&self, tag: Tag) -> Result<DeadlineGuard, Error> {
        let mut deadline = self.get_deadline(tag)?;
        deadline.start()?;

        Ok(DeadlineGuard(deadline))
    }

    /// Creates a custom [`Deadline`] from a range.
    pub fn create_custom_deadline(&self, range: DurationRange) -> Result<Deadline, Error> {
        // Not checking if status is already Failed, because it locks a mutex and is thus heavy at runtime.

        // TODO: Not sure. Maybe this should be done at the range level.
        if range.max < range.min {
            return Err(Error::BadParameter);
        }

        Ok(Deadline {
            inner: Arc::clone(&self.inner),
            range,
            handle: None,
            min_elapsed: Duration::ZERO,
            max_elapsed: Duration::ZERO,
        })
    }

    /// Creates a custom [`DeadlineGuard`] from a range.
    pub fn create_custom_deadline_guard(&self, range: DurationRange) -> Result<DeadlineGuard, Error> {
        let mut deadline = self.create_custom_deadline(range)?;
        deadline.start()?;

        Ok(DeadlineGuard(deadline))
    }

    /// Enables the monitor if it was previosuly disabled via [`DeadlineMonitor::disable`].
    ///
    /// Returns [`Error::NotAllowed`] if not disabled.
    pub fn enable(&self) -> Result<(), Error> {
        // The atomic status is protected by the timer mutex because they need to remain in sync.
        let mut timer = self.inner.timer.lock().unwrap();

        if self.inner.status.compare_exchange(Status::Disabled.into(), Status::Running.into(),
                Ordering::Relaxed, Ordering::Relaxed).is_ok()
        {
            timer.start();
            Ok(())
        } else {
            Err(Error::NotAllowed)
        }
    }

    /// Disables the monitor if running.
    ///
    /// Returns [`Error::NotAllowed`] if not running.
    pub fn disable(&self) -> Result<(), Error> {
        // The atomic status is protected by the timer mutex because they need to remain in sync.
        let mut timer = self.inner.timer.lock().unwrap();

        if self.inner.status.compare_exchange(Status::Running.into(), Status::Disabled.into(),
                Ordering::Relaxed, Ordering::Relaxed).is_ok()
        {
            timer.stop();
            Ok(())
        } else {
            Err(Error::NotAllowed)
        }
    }

    /// Retrieves the current status of the monitor.
    pub fn status(&self) -> Status {
        self.inner.status.load(Ordering::Relaxed).into()
    }

    pub(crate) fn evaluate(&self) -> Status {
        if self.status() == Status::Running {
            let elapsed = self.inner.timer.lock().unwrap().elapsed();
            self.inner.container.get_state().for_each(|_, deadline: &ActiveDeadline| {
                if elapsed > deadline.max_elapsed {
                    self.inner.status.store(Status::Failed.into(), Ordering::Relaxed);
                    CallbackProgression::Stop
                } else {
                    CallbackProgression::Continue
                }
            });
        }

        self.inner.status.load(Ordering::Relaxed).into()
    }
}

pub struct Deadline {
    inner: Arc<Inner>,
    range: DurationRange,
    handle: Option<ContainerHandle>,
    min_elapsed: Duration,
    max_elapsed: Duration,
}

impl Deadline {
    /// Starts the deadline.
    ///
    /// The deadline will be monitored by its [`DeadlineMonitor`] until stopped.
    ///
    /// Returns [`Error::NotAllowed`] if already started.
    /// Returns [`Error::OutOfMemory`] if too many deadlines are being monitored already.
    pub fn start(&mut self) -> Result<(), Error> {
        if self.handle.is_none() {
            let elapsed = self.inner.timer.lock().unwrap().elapsed();

            self.min_elapsed = elapsed + self.range.min;
            self.max_elapsed = elapsed + self.range.max;

            let res = unsafe {
                self.inner.container.add(ActiveDeadline {
                    max_elapsed: self.max_elapsed,
                })
            };

            match res {
                Ok(handle) => {
                    self.handle = Some(handle);
                    Ok(())
                }
                Err(_) => Err(Error::OutOfMemory),
            }
        } else {
            Err(Error::NotAllowed)
        }
    }

    /// Stops and evaluates the deadline.
    ///
    /// Returns [`Error::NotAllowed`] if not started.
    /// Returns [`Error::Generic`] if the deadline has expired and the monitor is running or already failed.
    pub fn stop(&mut self) -> Result<(), Error> {
        if let Some(handle) = self.handle.take() {
            unsafe { self.inner.container.remove(handle, ReleaseMode::Default) };

            let elapsed = self.inner.timer.lock().unwrap().elapsed();
            if elapsed < self.min_elapsed || elapsed > self.max_elapsed {
                // Report an error if the monitor either was running or already failed.
                match self.inner.status.compare_exchange(Status::Running.into(), Status::Failed.into(),
                        Ordering::Relaxed, Ordering::Relaxed)
                {
                    Ok(_) => return Err(Error::Generic),
                    Err(status) if status == Status::Failed.into()  => return Err(Error::Generic),
                    _ => ()
                }
            }

            Ok(())
        } else {
            Err(Error::NotAllowed)
        }
    }

    /// Returns the range of the deadline.
    pub fn range(&self) -> DurationRange {
        self.range
    }
}

impl Drop for Deadline {
    fn drop(&mut self) {
        debug_assert!(self.handle.is_none(), "Deadline dropped while started.");
    }
}

/// A [`Deadline`] that is created already started, and stops on drop.
pub struct DeadlineGuard(Deadline);

impl DeadlineGuard {
    /// Returns the range of the underlying [`Deadline`].
    pub fn range(&self) -> DurationRange {
        self.0.range
    }
}

impl Drop for DeadlineGuard {
    fn drop(&mut self) {
        // TODO: Error handling? panic?
        let _ = self.0.stop();
    }
}

struct Timer {
    elapsed: Duration,
    started_at: Option<Instant>,
}

impl Timer {
    fn new() -> Self {
        Self {
            elapsed: Duration::ZERO,
            started_at: Some(Instant::now()),
        }
    }

    fn start(&mut self) {
        if self.started_at.is_none() {
            self.started_at = Some(Instant::now());
        } else {
            // Error: Already started.
        }
    }

    fn stop(&mut self) {
        if let Some(started_at) = self.started_at {
            self.elapsed += Instant::now().duration_since(started_at);
            self.started_at = None;
        } else {
            // Error: Already stopped.
        }
    }

    fn elapsed(&self) -> Duration {
        if let Some(started_at) = self.started_at {
            self.elapsed + Instant::now().duration_since(started_at)
        } else {
            self.elapsed
        }
    }
}

struct Inner {
    container: FixedSizeContainer<ActiveDeadline, 256_usize>,
    status: AtomicU32,
    timer: Mutex<Timer>,
    templates: HashMap<Tag, DeadlineTemplate>,
}

// Safety: All fields except for templates are Send + Sync. templates is read-only.
unsafe impl Send for Inner {}
unsafe impl Sync for Inner {}

#[derive(Clone, Copy, Debug)]
struct ActiveDeadline {
    max_elapsed: Duration,
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::{thread, time::Duration};

    #[test]
    fn disable_enable() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");

        assert_eq!(deadline_monitor.status(), Status::Running);
        assert_eq!(deadline_monitor.disable(), Ok(()));
        assert_eq!(deadline_monitor.status(), Status::Disabled);
        assert_eq!(deadline_monitor.enable(), Ok(()));
        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn enable_while_enabled() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");

        assert_eq!(deadline_monitor.status(), Status::Running);
        assert_eq!(deadline_monitor.enable(), Err(Error::NotAllowed));
        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn disable_while_disabled() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");

        assert_eq!(deadline_monitor.status(), Status::Running);
        assert_eq!(deadline_monitor.disable(), Ok(()));
        assert_eq!(deadline_monitor.status(), Status::Disabled);
        assert_eq!(deadline_monitor.disable(), Err(Error::NotAllowed));
        assert_eq!(deadline_monitor.status(), Status::Disabled);
        assert_eq!(deadline_monitor.enable(), Ok(()));
        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn min_larger_than_max() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");

        let create_err = deadline_monitor
            .create_custom_deadline(DurationRange::from_millis(100, 99))
            .err();
        assert!(create_err.is_some());
        assert_eq!(create_err.unwrap(), Error::BadParameter);
    }

    //
    // Added deadline tests
    //

    #[test]
    fn deadline_met() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_deadline("test_deadline".into(), DurationRange::from_millis(10, 1000));
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .get_deadline("test_deadline".into())
                .unwrap();

            deadline.start().expect("Failed to start.");
            thread::sleep(Duration::from_millis(250));
            deadline.stop().expect("Failed to stop.");
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn deadline_too_slow() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_deadline("test_deadline".into(), DurationRange::from_millis(10, 1000));
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .get_deadline("test_deadline".into())
                .unwrap();

            deadline.start().expect("Failed to start.");
            thread::sleep(Duration::from_millis(2000));
            assert_eq!(deadline.stop(), Err(Error::Generic));
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }

    #[test]
    fn deadline_too_fast() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_deadline("test_deadline".into(), DurationRange::from_millis(100, 1000));
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .get_deadline("test_deadline".into())
                .unwrap();

            deadline.start().expect("Failed to start.");
            assert_eq!(deadline.stop(), Err(Error::Generic));
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }

    #[test]
    fn deadline_too_slow_while_disabled() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_deadline("test_deadline".into(), DurationRange::from_millis(10, 1000));
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        deadline_monitor.disable().expect("Failed to disable.");
        assert_eq!(deadline_monitor.status(), Status::Disabled);

        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .get_deadline("test_deadline".into())
                .unwrap();

            deadline.start().expect("Failed to start.");
            thread::sleep(Duration::from_millis(2000));
            deadline
                .stop()
                .expect("Failed to stop even though disabled.");
        });

        let _ = t1.join();

        deadline_monitor.enable().expect("Failed to enable.");
        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn deadline_too_fast_while_disabled() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_deadline("test_deadline".into(), DurationRange::from_millis(100, 1000));
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        deadline_monitor.disable().expect("Failed to disable.");
        assert_eq!(deadline_monitor.status(), Status::Disabled);

        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .get_deadline("test_deadline".into())
                .unwrap();

            deadline.start().expect("Failed to start.");
            deadline
                .stop()
                .expect("Failed to stop even though disabled.");
        });

        let _ = t1.join();

        deadline_monitor.enable().expect("Failed to enable.");
        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn deadline_guard_met() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_deadline("test_deadline".into(), DurationRange::from_millis(10, 1000));
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let _guard = deadline_monitor_clone
                .get_deadline_guard("test_deadline".into())
                .unwrap();

            thread::sleep(Duration::from_millis(250));
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn deadline_guard_too_slow() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_deadline("test_deadline".into(), DurationRange::from_millis(10, 1000));
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let _guard = deadline_monitor_clone
                .get_deadline_guard("test_deadline".into())
                .unwrap();

            thread::sleep(Duration::from_millis(2000));
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }

    #[test]
    fn deadline_guard_too_fast() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_deadline("test_deadline".into(), DurationRange::from_millis(100, 1000));
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let _guard = deadline_monitor_clone
                .get_deadline_guard("test_deadline".into())
                .unwrap();
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }

    #[test]
    fn stop_new_deadline() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_deadline("test_deadline".into(), DurationRange::from_millis(0, 1000));
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let mut deadline = deadline_monitor
            .get_deadline("test_deadline".into())
            .unwrap();

        assert_eq!(deadline.stop(), Err(Error::NotAllowed));
    }

    #[test]
    fn start_started_deadline() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_deadline("test_deadline".into(), DurationRange::from_millis(0, 1000));
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let mut deadline = deadline_monitor
            .get_deadline("test_deadline".into())
            .unwrap();

        assert_eq!(deadline.start(), Ok(()));
        assert_eq!(deadline.start(), Err(Error::NotAllowed));
        assert_eq!(deadline.stop(), Ok(()));
    }

    #[test]
    fn evaluated_deadline_too_slow() {
        let mut deadline_monitor_builder = DeadlineMonitorBuilder::new();
        deadline_monitor_builder.add_deadline("test_deadline".into(), DurationRange::from_millis(10, 1000));
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");

        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .get_deadline("test_deadline".into())
                .unwrap();

            deadline.start().expect("Failed to start.");
            thread::sleep(Duration::from_millis(2000));
            assert_eq!(deadline.stop(), Err(Error::Generic));
        });

        let deadline_monitor_clone = deadline_monitor.clone();
        let t2 = thread::spawn(move || {
            while deadline_monitor_clone.evaluate() != Status::Failed {
                thread::sleep(Duration::from_millis(100));
            }
        });

        let _ = t1.join();
        let _ = t2.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }


    //
    // Custom deadline tests
    //

    #[test]
    fn custom_deadline_met() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .create_custom_deadline(DurationRange::from_millis(10, 1000))
                .unwrap();

            deadline.start().expect("Failed to start.");
            thread::sleep(Duration::from_millis(250));
            deadline.stop().expect("Failed to stop.");
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn custom_deadline_too_slow() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .create_custom_deadline(DurationRange::from_millis(10, 1000))
                .unwrap();

            deadline.start().expect("Failed to start.");
            thread::sleep(Duration::from_millis(2000));
            assert_eq!(deadline.stop(), Err(Error::Generic));
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }

    #[test]
    fn custom_deadline_too_fast() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .create_custom_deadline(DurationRange::from_millis(100, 1000))
                .unwrap();

            deadline.start().expect("Failed to start.");
            assert_eq!(deadline.stop(), Err(Error::Generic));
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }

    #[test]
    fn custom_deadline_too_slow_while_disabled() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        deadline_monitor.disable().expect("Failed to disable.");
        assert_eq!(deadline_monitor.status(), Status::Disabled);

        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .create_custom_deadline(DurationRange::from_millis(10, 1000))
                .unwrap();

            deadline.start().expect("Failed to start.");
            thread::sleep(Duration::from_millis(2000));
            deadline
                .stop()
                .expect("Failed to stop even though disabled.");
        });

        let _ = t1.join();

        deadline_monitor.enable().expect("Failed to enable.");
        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn custom_deadline_too_fast_while_disabled() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        deadline_monitor.disable().expect("Failed to disable.");
        assert_eq!(deadline_monitor.status(), Status::Disabled);

        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .create_custom_deadline(DurationRange::from_millis(100, 1000))
                .unwrap();

            deadline.start().expect("Failed to start.");
            deadline
                .stop()
                .expect("Failed to stop even though disabled.");
        });

        let _ = t1.join();

        deadline_monitor.enable().expect("Failed to enable.");
        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn custom_deadline_guard_met() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let _guard = deadline_monitor_clone
                .create_custom_deadline_guard(DurationRange::from_millis(10, 1000))
                .unwrap();

            thread::sleep(Duration::from_millis(250));
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Running);
    }

    #[test]
    fn custom_deadline_guard_too_slow() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let _guard = deadline_monitor_clone
                .create_custom_deadline_guard(DurationRange::from_millis(10, 1000))
                .unwrap();

            thread::sleep(Duration::from_millis(2000));
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }

    #[test]
    fn custom_deadline_guard_too_fast() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let _guard = deadline_monitor_clone
                .create_custom_deadline_guard(DurationRange::from_millis(100, 1000))
                .unwrap();
        });

        let _ = t1.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }

    #[test]
    fn stop_new_custom_deadline() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let mut deadline = deadline_monitor
            .create_custom_deadline(DurationRange::from_millis(0, 1000))
            .unwrap();

        assert_eq!(deadline.stop(), Err(Error::NotAllowed));
    }

    #[test]
    fn start_started_custom_deadline() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");
        let mut deadline = deadline_monitor
            .create_custom_deadline(DurationRange::from_millis(0, 1000))
            .unwrap();

        assert_eq!(deadline.start(), Ok(()));
        assert_eq!(deadline.start(), Err(Error::NotAllowed));
        assert_eq!(deadline.stop(), Ok(()));
    }

    #[test]
    fn evaluated_custom_deadline_too_slow() {
        let deadline_monitor_builder = DeadlineMonitorBuilder::new();
        let deadline_monitor = deadline_monitor_builder
            .build()
            .expect("Failed to build the monitor.");

        let deadline_monitor_clone = deadline_monitor.clone();
        let t1 = thread::spawn(move || {
            let mut deadline = deadline_monitor_clone
                .create_custom_deadline(DurationRange::from_millis(10, 1000))
                .unwrap();

            deadline.start().expect("Failed to start.");
            thread::sleep(Duration::from_millis(2000));
            assert_eq!(deadline.stop(), Err(Error::Generic));
        });

        let deadline_monitor_clone = deadline_monitor.clone();
        let t2 = thread::spawn(move || {
            while deadline_monitor_clone.evaluate() != Status::Failed {
                thread::sleep(Duration::from_millis(100));
            }
        });

        let _ = t1.join();
        let _ = t2.join();

        assert_eq!(deadline_monitor.status(), Status::Failed);
    }
}
