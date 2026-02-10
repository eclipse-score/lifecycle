# Overview

Portable and high-performance implementation of a Lifecycle feature for S-CORE project.

This repository contains source code for Launch Manager and Health Monitor. Lifecycle feature is implemented in C++ language, but Rust language bindings are also provided. Additionally, to demonstrate how to use Lifecycle, a set of example applications can be found in ``demo`` folder.

High level functionality provided by Lifecycle:

* **Launch Manager**
    * **Portability**: Compatible with multiple operating systems including Linux, QNX7, and QNX8.
    * **Process Group Management**: Applications can be grouped into ProcessGroups, which are managed collectively as a single unit.
    * **Startup and Shutdown Dependencies**: The order of application startup and shutdown is determined by predefined dependency configurations.
    * **Failure Recovery**: Recovery actions are initiated upon detection of abnormal process termination.
* **Health Monitor**
    * **Supervision Types**: Supports Alive supervision, Deadline supervision, and Logical supervision.
    * **Failure Recovery**: Recovery actions are requested to Launch Manager when supervision failures are detected.
    * **Configurable Detection Timing**: The maximum failure detection time can be adjusted via a tunable supervision evaluation cycle.
    * **External Watchdog Integration**: Compatible with external watchdogs through configurable watchdog device file.

![Lifecycle Management Overview](images/lcm_overview.png)

# Getting Started

This section contains information on how to build and use Lifecycle feature.

## Prerequisites

* C++ Compiler: gcc/clang with C++17 support
* Rust Version: 1.85 (edition 2021)
* Build System: Bazel
* Operating System: Linux (Ubuntu 22.04+)
* Dependencies: S-Core Baselibs, Google Flatbuffers, libacl1-dev
* Dependencies of demo applications: Docker, Python (3.12)

## Building the project

It is recommended to use the devcontainer for building the project, see [eclipse-score/devcontainer/README.md#inside-the-container](https://github.com/eclipse-score/devcontainer/blob/main/README.md#inside-the-container)

Build all components for **Linux** by running

```sh
bazel build --config=x86_64-linux //...
```

To build launch_manager and health_monitor daemons with the sanitizers enabled use one of the following

```
bazel build --define sanitize=thread //...

bazel build --define sanitize=address //...

bazel build --define sanitize=undefined //...
```

To build all components with ``score::mw::log`` enabled, use this command:

```sh
bazel build --cxxopt=-DLC_LOG_SCORE_MW_LOG //...
```

You can also use the config `--config=x86_64-linux` to build for linux.

## IDE support

### C++
Use Visual Studio Code with `clangd`. Make sure you don't have the MS C++ IntelliSense extension installed as this is likely to clash with `clangd`.
Then you need to call `./scripts/generate_cpp_compile_commands.sh` to generate compilation DB for clangd and restart it in VS Code. Indexing shall work.

### Rust


### QNX

#### Envionment Setup
Either add to ~/.netrc
```sh
machine qnx.com
login <qnx email>
password <qnx password>
```
(Makes sure your netrc has the right permissions `chmod 600 ~/.netrc`)

Or export env vars:
```sh
export SCORE_QNX_USER=<qnx email>
export SCORE_QNX_PASSWORD=<qnx password>
```
Also if you are not running on the devcontainer make sure the qnx license
is in the correct directory:

```sh
sudo mkdir /opt/score_qnx/license
cp -r ~/.qnx/license/* /opt/score_qnx/license
sudo chmod 777 -R /opt/score_qnx/
```

Next make the `~/.bazelrc` that contains the following:
`common --action_env=QNXLM_LICENSE_FILE=<License server / file>`

#### Build

##### Fetch QNX SDP

Note: For the next command if you are getting checksum mismatch errors then its
possible you need to agree the terms and condition of the SDP by clicking the
URL in the error message and completing the form.

```sh
bazel fetch @score_toolchains_qnx//...
cred_helper="$(find -L $(bazel info output_base)/external -name qnx_credential_helper.py -print -quit)"
bazel fetch @toolchains_qnx_sdp//... --credential_helper=*.qnx.com="$cred_helper"
```

##### Build For QNX

```sh
bazel build //launch_manager/... --config=x86_64-qnx
bazel build //health_monitor/... --config=x86_64-qnx
```

TODO: Currently rust binaries are not compiling for QNX.

## Running Lifecycle feature and example applications

The ``demo`` folder contains a demo setup running in a docker container, consisting of a set of example applications and corresponding configuration. As per configuration, Launch Manager will start Health Monitor and a set of configured applications. For more information see [demo/README.md](demo/README.md) file.

## Configuration

Lifecycle feature is configured by json files.

Following json code demonstrates Launch Manager configuration. Key elements of the configuration:

  * A single process group is configured (``MainPG``), with three states.
  * A single process is configured with ``LD_LIBRARY_PATH`` environmental variable and no command line arguments.
  * The configured process will run in ``Startup`` and ``Recovery`` state.

```json
{
  "versionMajor": 7,
  "versionMinor": 0,
  "Process": [
    {
      "identifier": "healthmonitor",
      "uid": 0,
      "gid": 0,
      "path": "/opt/health_monitor/health_monitor",
      "functionClusterAffiliation": "STATE_MANAGEMENT",
      "numberOfRestartAttempts": 0,
      "executable_reportingBehavior": "ReportsExecutionState",
      "sgids": [],
      "startupConfig": [
        {
          "executionError": "1",
          "schedulingPolicy": "SCHED_OTHER",
          "schedulingPriority": "0",
          "identifier": "health_monitor_startup_config",
          "enterTimeoutValue": 20000,
          "exitTimeoutValue": 1000,
          "terminationBehavior": "ProcessIsNotSelfTerminating",
          "executionDependency": [],
          "processGroupStateDependency": [
            {
              "stateMachine_name": "MainPG",
              "stateName": "MainPG/Startup"
            },
            {
              "stateMachine_name": "MainPG",
              "stateName": "MainPG/Recovery"
            }
          ],
          "environmentVariable": [
            {
              "key": "LD_LIBRARY_PATH",
              "value": "/opt/lib"
            }
          ],
          "processArgument": []
        }
      ]
    }
  ],
  "ModeGroup": [
    {
      "identifier": "MainPG",
      "initialMode_name": "Off",
      "recoveryMode_name": "MainPG/Recovery",
      "modeDeclaration": [
        {
          "identifier": "MainPG/Off"
        },
        {
          "identifier": "MainPG/Startup"
        },
        {
          "identifier": "MainPG/Recovery"
        }
      ]
    }
  ]
}
```

For the convenience of the end user, a python module is provided for Launch Manager. Following code demonstrates Launch Manager configuration. Key elements of the configuration:

  * A single machine is configured with, global ``LD_LIBRARY_PATH`` environmental variable.
  * A single process group is configured (``MainPG``), with three states.
  * 50 processes are configured, each with unique UID and GID.
    * All processes are started from the same executable.
    * All processes will be started in ``Startup`` state of ``MainPG`` process group.
    * Each process has its own ``PROCESSIDENTIFIER`` environmental variable. Value of that variable is different for each process.
    * Only second process has command line argument configured.

```python
conf_gen = LaunchManagerConfGen()
qt_am_machine = conf_gen.add_machine("qt_am_machine",
                                     env_variables = {"LD_LIBRARY_PATH": "/opt/lib"})

pg_machine = conf_gen.machine_add_process_group(qt_am_machine,
                                                "MainPG", ["Off", "Startup", "Recovery"])

for i in range(50):
    cpp_lifecycle_app = conf_gen.process_group_add_process(pg_machine,
                                                           f"{process_group_name}_cpp_lifecycle_app{i}",
                                                           executable_name = "/opt/cpp_lifecycle_app/cpp_lifecycle_app",
                                                           uid = 1000 + i,
                                                           gid = 1000 + i)
    conf_gen.process_add_startup_config(cpp_lifecycle_app,
                                        f"cpp_lifecycle_app_startup_config_{i}",
                                        process_arguments = ["-c", "2000"] if i == 1 else [],
                                        env_variables = {"PROCESSIDENTIFIER": f"{process_group_name}_lc{i}"},
                                        scheduling_policy = "SCHED_OTHER",
                                        scheduling_priority = 0,
                                        enter_timeout = 5.0,
                                        exit_timeout = 2.0,
                                        use_in = ["Startup"])

conf_gen.generate_json("./lm_demo.json")
```

Following json code demonstrates Health Monitor configuration. Key elements of the configuration:

  * Single process is supervised.
  * Alive, deadline and logical supervision is configured for that process.
  * Single process group is configured.
  * Recovery action for that process group is configured.

Sample configuration of the monitored application:
```json
{
    "versionMajor": 8,
    "versionMinor": 0,
    "process": [],
    "hmMonitorInterface": [
        {
            "instanceSpecifier": "demo/demo_application0/Port1",
            "processShortName": "demo_application0",
            "portPrototype": "Port1",
            "interfacePath": "demo_application_0_MainPG",
            "refProcessIndex":0
        }
    ]
}
```

Sample configuration of Health Monitor daemon:
```json
{
    "versionMajor": 8,
    "versionMinor": 0,
    "process": [
        {
            "index": 0,
            "shortName": "demo_application0",
            "identifier": "demo_app0_MainPG",
            "processType": "REGULAR_PROCESS",
            "refProcessGroupStates": [
                {
                    "identifier": "MainPG/Startup"
                }
            ],
            "processExecutionErrors": [
                {
                    "processExecutionError": 1
                }
            ]
        }
    ],
    "hmMonitorInterface": [
        {
            "instanceSpecifier": "demo/demo_application0/Port1",
            "processShortName": "demo_application0",
            "portPrototype": "Port1",
            "interfacePath": "demo_application_0_MainPG",
            "refProcessIndex": 0,
            "permittedUid": 0
        }
    ],
    "hmSupervisionCheckpoint": [
        {
            "shortName": "Checkpoint0_1",
            "checkpointId": 1,
            "refInterfaceIndex": 0
        },
        {
            "shortName": "Checkpoint0_2",
            "checkpointId": 2,
            "refInterfaceIndex": 0
        },
        {
            "shortName": "Checkpoint0_3",
            "checkpointId": 3,
            "refInterfaceIndex": 0
        }
    ],
    "hmAliveSupervision": [
        {
            "ruleContextKey": "AliveSupervision0",
            "refCheckPointIndex": 0,
            "aliveReferenceCycle": 100.0,
            "minAliveIndications": 1,
            "maxAliveIndications": 3,
            "isMinCheckDisabled": false,
            "isMaxCheckDisabled": false,
            "failedSupervisionCyclesTolerance": 1,
            "refProcessIndex": 0,
            "refProcessGroupStates": [
                {
                    "identifier": "MainPG/Startup"
                }
            ]
        }
    ],
    "hmDeadlineSupervision": [
        {
            "ruleContextKey": "DeadlineSupervision0",
            "maxDeadline": 80.0,
            "minDeadline": 40.0,
            "checkpointTransition": {
                "refSourceCPIndex": 0,
                "refTargetCPIndex": 1
            },
            "refProcessIndices": [
                0
            ],
            "refProcessGroupStates": [
                {
                    "identifier": "MainPG/Startup"
                }
            ]
        }
    ],
    "hmLogicalSupervision": [
        {
            "ruleContextKey": "LogicalSupervision0",
            "checkpoints": [
                {
                    "refCheckPointIndex": 0,
                    "isInitial": true,
                    "isFinal": false
                },
                {
                    "refCheckPointIndex": 1,
                    "isInitial": false,
                    "isFinal": false
                },
                {
                    "refCheckPointIndex": 2,
                    "isInitial": false,
                    "isFinal": true
                }
            ],
            "transitions": [
                {
                    "checkpointSourceIdx": 0,
                    "checkpointTargetIdx": 1
                },
                {
                    "checkpointSourceIdx": 1,
                    "checkpointTargetIdx": 2
                }
            ],
            "refProcessIndices": [
                0
            ],
            "refProcessGroupStates": [
                {
                    "identifier": "MainPG/Startup"
                }
            ]
        }
    ],
    "hmLocalSupervision": [
        {
            "ruleContextKey": "LocalSupervision0",
            "infoRefInterfacePath": "demo_application_0",
            "hmRefAliveSupervision": [
                {
                    "refAliveSupervisionIdx": 0
                }
            ],
            "hmRefDeadlineSupervision": [
                {
                    "refDeadlineSupervisionIdx": 0
                }
            ],
            "hmRefLogicalSupervision": [
                {
                    "refLogicalSupervisionIdx": 0
                }
            ]
        }
    ],
    "hmGlobalSupervision": [
        {
            "ruleContextKey": "GlobalSupervision_MainPG",
            "isSeverityCritical": false,
            "localSupervision": [
                {
                    "refLocalSupervisionIndex": 0
                }
            ],
            "refProcesses": [
                {
                    "index": 0
                }
            ],
            "refProcessGroupStates": [
                {
                    "identifier": "MainPG/Startup"
                }
            ]
        }
    ],
    "hmRecoveryNotification": [
        {
            "shortName": "RecoveryNotification_MainPG",
            "recoveryNotificationTimeout": 4000.0,
            "processGroupMetaModelIdentifier": "MainPG/Recovery",
            "refGlobalSupervisionIndex": 0,
            "instanceSpecifier": "",
            "shouldFireWatchdog": false
        }
    ]
}
```

Full configuration for example applications can be found in ``demo/config`` folder.

# Architecture

The Lifecycle feature is divided in to two logical components, Launch Manager and Health Monitor. In our solution each component is implemented as a separate daemon.

## Launch Manager architecture

Following diagram shows high level overview of Launch Manager architecture.
![Launch Manager architecture overview](images/lm_arch_overview.png)

Main components are:
  * **Control client library:** Library used to control states of Process Groups. Each Process Group has a set of states that could be activated or switched off. This library can be used to control states of Process Groups from C++ or Rust application. Please note that by controlling Process Group states, we are starting or stopping applications. Rights to use this library has to be assigned during configuration step, otherwise an attempt to use this interface will be blocked.
  * **Lifecycle client library:** Library used by S-CORE applications, to report application states back to Launch Manager. Every non native application should use this interface. This interface is available in C++ and Rust language.
  * **Configuration manager:** Small portability layer, designed to protect business logic from changes in configuration format. Current implementation reads data from FlatBuffers files and populate internal data structures.
  * **OS abstraction layer (OSAL):** Portability layer designed to protect business logic, from differences in POSIX based OSs. At the moment we support ``Linux`` based OSs and ``QNX``. It is worth to note, that we follow ``fork`` then ``exec`` path, when starting new processes. This approach provides greater flexibility than the ``spawn`` method, particularly when setting up the execution environment and access rights for child processes.
  * **Process Group manager:** Business logic is implemented in Process Group manager. Following diagram shows high level overview, of this component.

![Process Group manager architecture overview](images/process_group_manager_arch_overview.png)

Key components of Process Group manager are:
  * **OS Handler:** A simple thread tasked with collecting exit status of child processes. As soon as exit status of a child process becomes available, we collect it and pass that information to the Process Group manager.
  * **Process Group:** Graph that holds information about processes that belongs to a particular Process Group. This is the place where we store start-up configuration, the information on how to start a process, as well as information on when to start a process.
  * **Thread pool / Job queue:** This is our scalability engine. By controlling queue size and number of threads, we can scale up and scale down (in line with hardware capabilities we are running on).
  * **Worker thread:** A simple wrapper around POSIX thread. It will fetch a job from a queue, if there is anything to be done, or sleep otherwise.
  * **Process Group Manager:** This is where main business logic is implemented. This thread will service all external communication channels and if there is a Process Group state change requested, it will add initial process start to the queue. When thread pool process all start / stop operations, this component will send response to the requesting Control client library. If a Process Group end up in an error state, and there is no active request from outside, Process Group manager will execute a recovery action.

## Health Monitor architecture

Following diagram shows high level overview of Health Monitor architecture.
![Health Monitor architecture overview](images/hm_arch_overview.png)

  * **Health Monitor library (hm lib):** Library used by supervised applications to report checkpoints, that have been configured for monitoring. This library executes without a background thread, so we can avoid forced context switches when application reports a checkpoint.
  * **Configuration:** Portability layer designed to shield business logic from changes in configuration format. Its implementation follows factory design pattern. We read data from FlatBuffers files and return fully initialized objects.
  * **Supervision:** This component periodically reads all reported checkpoints from shared memory and evaluates supervision status for each configured process. Upon detection of an error condition, the recovery component is triggered to initiate appropriate corrective actions.
  * **Recovery:** When an error is detected by supervision component, recovery component will perform preconfigured recovery action.
  * **Watchdog:** This component is responsible for the initialization, control, and servicing of the external watchdog.

The Health Monitor is implemented as a single-threaded process. The frequency at which the supervision component evaluates reported checkpoints can be configured via a dedicated parameter. This configurability enables optimization of CPU utilization against worst-case fault detection latency. Depending on the target hardware platform, a trade-off can be made between detection time and CPU consumption.

The external watchdog abstraction layer is implemented using the ``/dev/watchdog`` device interface, which is commonly available on Linux-based operating systems. For QNX environments, the same interface can be implemented using a resource manager.


# Requirements coverage

This overview represent a high level illustration of requirements coverage and is not a traceability view. Full list of requirement can be found here: [Lifecycle Requirements](https://eclipse-score.github.io/score/main/features/lifecycle/requirements/index.html).

## ✅ Launch Manager features

### Process management Capabilities

    - Launch processes with dependency ordering ✅
    - Parallel execution & Multiple instances of executables ✅
    - UID/GID, Security policy and scheduler configuration ✅
    - Timeouts & Retry on process startup ✅
    - Validate dependency consistency (Tooling / startup) ⏳
    - OCI compliance, ASLR support & process adoptions ⏳

### Run targets & Conditional Launching

    - Define, Launch & Switch between named run targets ✅
    - Launch based on conditions (run targets / dependencies), Debug mode ✅
    - Configurable timeouts and polling intervals for conditions ⏳
    - Pre/post-start validation ⏳
    - Define process dependencies and stop sequences (dependency based) ✅

### Process Termination

    - Configurable stop timeout ✅
    - Terminate based on dependency order ✅
    - SIGTERM/SIGKILL delay ✅
    - Normal, slow, fast shutdown modes ⏳

### Control Interface

    - Allow external users to send control commands (activate, Stop, etc..) ✅
    - Request run target launch ✅
    - Component state reporting ⏳
    - Report & query component status (started/running/degraded) ⏳

## ✅ Health monitor features

### Monitoring & Recovery

    - Abnormal process termination detection ✅
    - Timing & logical monitoring ✅
    - Drop supervision ✅
    - Recovery actions (run target switch & process relaunch) ⏳
    - Configurable recovery wait time ✅
    - External monitor notifications & watchdog support ✅
    - Internal monitoring (self-health) checks ✅
