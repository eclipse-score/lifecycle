use crate::common::MonitorEvaluator;
use containers::fixed_capacity::FixedCapacityVec;

use crate::{
    common::MonitorEvalHandle,
    log::{info, warn},
};

/// An abstraction over the API used to notify the supervisor about process liveness.
pub(super) trait SupervisorAPIClient {
    fn notify_alive(&self);
}

pub(super) struct MonitoringLogic<T: SupervisorAPIClient> {
    monitors: FixedCapacityVec<MonitorEvalHandle>,
    client: T,
    last_notification: std::time::Instant,
    supervisor_api_cycle: core::time::Duration,
}

impl<T: SupervisorAPIClient> MonitoringLogic<T> {
    /// Creates a new MonitoringLogic instance.
    /// # Arguments
    /// * `monitors` - A vector of monitor evaluation handles.
    /// * `supervisor_api_cycle` - Duration between alive notifications to the supervisor.
    /// * `client` - An implementation of the SupervisorAPIClient trait.
    pub(super) fn new(
        monitors: FixedCapacityVec<MonitorEvalHandle>,
        supervisor_api_cycle: core::time::Duration,
        client: T,
    ) -> Self {
        Self {
            monitors,
            client,
            supervisor_api_cycle,
            last_notification: std::time::Instant::now(),
        }
    }

    fn run(&mut self) -> bool {
        let mut has_any_error = false;

        for monitor in self.monitors.iter() {
            monitor.evaluate(&mut |tag, error| {
                has_any_error = true;
                warn!("Monitor with tag {:?} reported error: {:?}.", tag, error);
            });
        }

        if !has_any_error {
            if self.last_notification.elapsed() > self.supervisor_api_cycle {
                self.last_notification = std::time::Instant::now();
                self.client.notify_alive();
            }
        } else {
            warn!("One or more monitors reported errors, skipping AliveAPI notification.");
            return false;
        }

        return true;
    }
}

/// A struct that manages a unique thread for running monitoring logic periodically.
pub struct UniqueThreadRunner {
    handle: Option<std::thread::JoinHandle<()>>,
    should_stop: std::sync::Arc<std::sync::atomic::AtomicBool>,
    internal_duration_cycle: core::time::Duration,
}

impl UniqueThreadRunner {
    pub(super) fn new(internal_duration_cycle: core::time::Duration) -> Self {
        Self {
            handle: None,
            should_stop: std::sync::Arc::new(std::sync::atomic::AtomicBool::new(false)),
            internal_duration_cycle,
        }
    }

    pub(super) fn start<T>(&mut self, mut monitoring_logic: MonitoringLogic<T>)
    where
        T: SupervisorAPIClient + Send + 'static,
    {
        self.handle = Some({
            let should_stop = self.should_stop.clone();
            let interval = self.internal_duration_cycle;

            std::thread::spawn(move || {
                info!("Monitoring thread started.");
                let mut next_sleep_time = interval;

                while !should_stop.load(std::sync::atomic::Ordering::Relaxed) {
                    std::thread::sleep(next_sleep_time);

                    let now = std::time::Instant::now();

                    if !monitoring_logic.run() {
                        info!("Monitoring logic failed, stopping thread.");
                        break;
                    }

                    next_sleep_time = interval - now.elapsed();
                }

                info!("Monitoring thread exiting.");
            })
        });
    }

    pub fn join(&mut self) {
        self.should_stop.store(true, std::sync::atomic::Ordering::Relaxed);
        if let Some(handle) = self.handle.take() {
            let _ = handle.join();
        }
    }
}

impl Drop for UniqueThreadRunner {
    fn drop(&mut self) {
        self.join();
    }
}

/// A stub implementation of the SupervisorAPIClient that logs alive notifications.
pub(super) struct StubSupervisorAPIClient;

impl SupervisorAPIClient for StubSupervisorAPIClient {
    fn notify_alive(&self) {
        warn!("StubSupervisorAPIClient: notify_alive called");
    }
}

#[score_testing_macros::test_mod_with_log]
#[cfg(test)]
mod tests {

    use crate::{
        deadline::{DeadlineMonitor, DeadlineMonitorBuilder},
        protected_memory::ProtectedMemoryAllocator,
        IdentTag, TimeRange,
    };

    use super::*;

    #[derive(Clone)]
    struct MockSupervisorAPIClient {
        pub notify_called: std::sync::Arc<std::sync::atomic::AtomicUsize>,
    }

    impl MockSupervisorAPIClient {
        pub fn new() -> Self {
            Self {
                notify_called: std::sync::Arc::new(std::sync::atomic::AtomicUsize::new(0)),
            }
        }

        fn get_notify_count(&self) -> usize {
            self.notify_called.load(std::sync::atomic::Ordering::Acquire)
        }
    }

    impl SupervisorAPIClient for MockSupervisorAPIClient {
        fn notify_alive(&self) {
            self.notify_called.fetch_add(1, std::sync::atomic::Ordering::AcqRel);
        }
    }

    fn create_monitor_with_deadlines() -> DeadlineMonitor {
        let allocator = ProtectedMemoryAllocator {};
        DeadlineMonitorBuilder::new()
            .add_deadline(
                &IdentTag::from("deadline_long"),
                TimeRange::new(core::time::Duration::from_secs(1), core::time::Duration::from_secs(50)),
            )
            .add_deadline(
                &IdentTag::from("deadline_fast"),
                TimeRange::new(
                    core::time::Duration::from_millis(0),
                    core::time::Duration::from_millis(50),
                ),
            )
            .build(&allocator)
    }

    #[test]
    fn monitoring_logic_report_error_when_deadline_failed() {
        let deadline_monitor = create_monitor_with_deadlines();
        let alive_mock = MockSupervisorAPIClient::new();

        let mut logic = MonitoringLogic::new(
            {
                let mut vec = FixedCapacityVec::new(2);
                vec.push(deadline_monitor.get_eval_handle()).unwrap();
                vec
            },
            core::time::Duration::from_secs(1),
            alive_mock.clone(),
        );

        let mut deadline = deadline_monitor.get_deadline(&IdentTag::from("deadline_long")).unwrap();
        let handle = deadline.start().unwrap();

        drop(handle);

        assert!(!logic.run());
        assert_eq!(alive_mock.get_notify_count(), 0);
    }

    #[test]
    fn monitoring_logic_report_alive_on_each_call_when_no_error() {
        let deadline_monitor = create_monitor_with_deadlines();
        let alive_mock = MockSupervisorAPIClient::new();

        let mut logic = MonitoringLogic::new(
            {
                let mut vec = FixedCapacityVec::new(2);
                vec.push(deadline_monitor.get_eval_handle()).unwrap();
                vec
            },
            core::time::Duration::from_nanos(0), // Make sure each call notifies alive
            alive_mock.clone(),
        );

        let mut deadline = deadline_monitor.get_deadline(&IdentTag::from("deadline_long")).unwrap();
        let _handle = deadline.start().unwrap();

        assert!(logic.run());
        assert!(logic.run());
        assert!(logic.run());
        assert!(logic.run());
        assert!(logic.run());

        assert_eq!(alive_mock.get_notify_count(), 5);
    }

    #[test]
    fn monitoring_logic_report_alive_respect_cycle() {
        let deadline_monitor = create_monitor_with_deadlines();
        let alive_mock = MockSupervisorAPIClient::new();

        let mut logic = MonitoringLogic::new(
            {
                let mut vec = FixedCapacityVec::new(2);
                vec.push(deadline_monitor.get_eval_handle()).unwrap();
                vec
            },
            core::time::Duration::from_millis(30),
            alive_mock.clone(),
        );

        let mut deadline = deadline_monitor.get_deadline(&IdentTag::from("deadline_long")).unwrap();
        let _handle = deadline.start().unwrap();

        std::thread::sleep(core::time::Duration::from_millis(30));
        assert!(logic.run());

        std::thread::sleep(core::time::Duration::from_millis(30));
        assert!(logic.run());

        std::thread::sleep(core::time::Duration::from_millis(30));
        assert!(logic.run());

        std::thread::sleep(core::time::Duration::from_millis(30));
        assert!(logic.run());

        std::thread::sleep(core::time::Duration::from_millis(30));
        assert!(logic.run());

        assert_eq!(alive_mock.get_notify_count(), 5);
    }

    #[test]
    fn unique_thread_runner_monitoring_works() {
        let deadline_monitor = create_monitor_with_deadlines();

        let alive_mock = MockSupervisorAPIClient::new();

        let logic = MonitoringLogic::new(
            {
                let mut vec = FixedCapacityVec::new(2);
                vec.push(deadline_monitor.get_eval_handle()).unwrap();
                vec
            },
            core::time::Duration::from_nanos(0), // Make sure each call notifies alive
            alive_mock.clone(),
        );

        let mut worker = UniqueThreadRunner::new(core::time::Duration::from_millis(10));
        worker.start(logic);

        let mut deadline = deadline_monitor.get_deadline(&IdentTag::from("deadline_fast")).unwrap();

        let handle = deadline.start().unwrap();

        std::thread::sleep(core::time::Duration::from_millis(70));

        let current_count = alive_mock.get_notify_count();
        assert!(
            current_count >= 1,
            "Expected at least 1 notify_alive call, got {}",
            current_count
        );

        // We shall not get any new alive calls.
        std::thread::sleep(core::time::Duration::from_millis(50));
        assert_eq!(alive_mock.get_notify_count(), current_count);
        handle.stop();
    }
}
