..
   # *******************************************************************************
   # Copyright (c) 2026 Contributors to the Eclipse Foundation
   #
   # See the NOTICE file(s) distributed with this work for additional
   # information regarding copyright ownership.
   #
   # This program and the accompanying materials are made available under the
   # terms of the Apache License Version 2.0 which is available at
   # https://www.apache.org/licenses/LICENSE-2.0
   #
   # SPDX-License-Identifier: Apache-2.0
   # *******************************************************************************


Launch Manager Configuration
############################

This document describes the configuration schema for the S-CORE **Launch Manager**, covering core concepts and providing detailed technical insights into how the **Launch Manager** configuration is structured and operates.

Introduction to Launch Manager Configuration
********************************************

Before diving into the specifics of the configuration structure, it is crucial to establish a common language and grasp how the **Launch Manager** orchestrates software components. This introduction is designed to equip new users with the basic understanding necessary to navigate the subsequent detailed configuration discussions.

The **Launch Manager** is an essential system component responsible for managing and orchestrating software units installed on a machine. This orchestration is primarily achieved through the use of **Run Targets**. To fully understand this process, we first need to define three key terms: **Components**, **Run Targets**, and the **Ready State**.

Components: The Building Blocks of Your System
==============================================

A **Component**, often referred to as a software component, is an independent, deployable software unit. Examples include applications, device drivers, or software containers. Components are typically developed in isolation and can then be deployed into various target systems, such as physical or virtual machines. During the deployment or integration phase, multiple components are often combined to form a complete, functioning system.

A single component might be deployed across several different systems. To facilitate this, the specific deployment configuration (how and where a component runs) is intentionally kept separate from the inherent properties of the component itself. This separation promotes flexibility and reusability.

Run Targets: Grouping and Components Activation
===============================================

A **Run Target** defines a specific collection of components. Essentially, it is a named group that lists the components intended to be active together. When a particular **Run Target** is activated, the **Launch Manager** performs the following sequence of operations to manage the components associated with it:

*   **Deactivation of Unassigned Components:** All components currently in the **Ready State** that are **not** part of the **Run Target** being activated will be deactivated.
*   **Activation of Assigned Components:** All components that are not currently in the **Ready State** but **are** assigned to the **Run Target** being activated will be started and brought to their **Ready State**.
*   **Maintenance of Active Assigned Components:** Any components already in the **Ready State** and also assigned to the **Run Target** being activated will remain active and unchanged.

Ready State: Confirmation of Operational Readiness
==================================================

Each component possesses a **Ready State**. This state signifies that the component has not only started successfully, but has also met all its configured ready conditions. The **Ready State** is crucial because it confirms that a component is fully operational and capable of performing its intended functions, allowing other dependent components, or the system as a whole, to confidently rely on its services.

It is vital to understand that a component's lifecycle, and consequently its **Ready State**, is distinct from the lifecycle of the underlying operating system process initiated during the component's startup sequence.

Consider an example: a script designed to mount a file system. When this component starts, the script will execute and complete its task as soon as the mount operation is finished. The component representing this script, however, will only reach its **Ready State** when the files within that file system become genuinely available for use by other processes. In this scenario, despite the script having finished its execution, the component remains in the **Ready State** because the file system is still mounted and accessible to the rest of the system. This illustrates how the **Ready State** reflects a component's functional availability, rather than merely the availability of its process.

When the Launch Manager Starts a Component
================================================

With the definitions of **Components**, **Run Targets**, and **Ready State** established, let us clarify the conditions under which the **Launch Manager** will initiate a component's startup sequence. Components will be started for two primary reasons:

*   A component is directly assigned to a **Run Target** that is currently being activated.
*   Another component, which is assigned to a **Run Target** being activated, explicitly depends on that component.

Understanding Dependencies: How Components and Run Targets Relate
=================================================================

A fundamental aspect of **Launch Manager** configuration involves understanding how components are assigned to a **Run Target** and how dependencies are declared.

When a component is said to be assigned to a **Run Target**, it means the **Run Target**'s configuration explicitly lists the component's name within its ``depends_on`` parameter.

Similarly, when a component depends on another component, its own ``component_properties.depends_on`` configuration parameter will list the name of the other component it requires.

Additionally, a **Run Target** can declare a dependency on another **Run Target**. In this scenario, the name of the dependent **Run Target** is listed within the ``depends_on`` configuration parameter of the primary **Run Target**. The most effective way to conceptualize this relationship is to imagine that the list of components assigned to the dependent **Run Target** is effectively included in the list of components of the primary **Run Target**.

Rules for Configuring Dependencies
==================================

When configuring dependencies within the **Launch Manager**, the following rules must be observed to ensure correct system behavior:

*   A **Component** can depend on another **Component**.
*   A **Run Target** can depend on a **Component**.
*   A **Run Target** can depend on another **Run Target**.
*   A **Component** cannot depend on a **Run Target**.

Having discussed these basic concepts and the fundamental operation of the **Launch Manager**, we are now ready to explore the detailed configuration structure.

Measurement units
*****************

This section provides an overview of the measurement units used within the configuration and explains how they ensure consistent representation of values within the system.

Time Intervals
==============

All time values in the **Launch Manager** configuration are specified in **seconds**. When a fraction of a second is required, a **decimal point** must be used.

For example:

*   ``0.5`` represents a time interval of 500 milliseconds.
*   ``1.5`` represents a time interval of 1500 milliseconds.

Using a consistent unit prevents ambiguity and makes the configuration values easier to compare and understand.

Memory
======

All configuration values representing memory sizes are specified in **bytes**.

For example:

*   ``1000`` represents one kilobyte (kB), following the **SI** standard.
*   ``1024`` represents one kibibyte (KiB), following the **IEC** standard.

Storing memory values in bytes ensures that all size-related settings remain precise and unambiguous.

Configuration schema
********************

This section provides a detailed description of the **Launch Manager** configuration schema. This section is organized to facilitate understanding, starting with common building blocks and then proceeding to the top-level configuration properties.

The **Launch Manager** configuration leverages reusable definitions, primarily found within the standard ``$defs`` object. These reusable types are the fundamental building blocks of the **Launch Manager** configuration. A thorough understanding of these types will simplify comprehension of the overall configuration structure and the interaction between different settings.

Reusable Types
==============

The following sections describe the reusable types that form the basis of the **Launch Manager** configuration.

alive_supervision (object)
--------------------------

**Description:**
  Defines a reusable type that contains configuration parameters for alive supervision, which helps monitor the health and responsiveness of components.

**Properties:**

*   **evaluation_cycle** (number, optional)
    *   **Description:** Specifies the length, in seconds (e.g., ``0.5`` for 500 milliseconds), of the time window used by the **Launch Manager** to assess incoming alive supervision reports from components.
    *   **Constraint:** Must be greater than 0.

watchdog (object)
-----------------

**Description:**
  Defines a reusable type that contains configuration parameters for the external watchdog device, used to monitor the overall system health and initiate resets if necessary.

**Properties:**

*   **device_file_path** (string, optional)
    *   **Description:** Specifies the absolute path to the external watchdog device file (e.g., ``/dev/watchdog``).
*   **max_timeout** (number, optional)
    *   **Description:** Specifies the maximum timeout value, in seconds (e.g., ``0.5`` for 500 milliseconds), that the **Launch Manager** configures on the external watchdog during startup. The external watchdog uses this timeout as the deadline for receiving periodic alive reports from the **Launch Manager**.
    *   **Constraint:** Must be 0 or greater.
*   **deactivate_on_shutdown** (boolean, optional)
    *   **Description:** Specifies whether the **Launch Manager** disables the external watchdog during shutdown. When set to ``true``, the watchdog is deactivated; when ``false``, it remains active, potentially triggering a reset if the shutdown is prolonged.
*   **require_magic_close** (boolean, optional)
    *   **Description:** Specifies whether the **Launch Manager** performs a defined shutdown sequence to inform the external watchdog that the shutdown is intentional and to prevent a watchdog-initiated reset. When ``true``, the magic close sequence is performed; when ``false``, it is not, which might lead to an unintentional reset.

recovery_action (object)
------------------------

**Description:**
  Defines a reusable type that specifies recovery actions to execute when an error or failure occurs. This object must contain exactly one of the defined recovery actions.

**Properties:**

*   **restart** (object, optional)
    *   **Description:** Defines a recovery action that restarts the POSIX process associated with this component.
    *   **Properties:**
        *   **number_of_attempts** (integer, optional)
            *   **Description:** Specifies the maximum number of restart attempts before the **Launch Manager** concludes that recovery cannot succeed for the component.
            *   **Constraint:** Must be 0 or greater.
        *   **delay_before_restart** (number, optional)
            *   **Description:** Specifies the delay duration, in seconds (e.g., ``0.25`` for 250 milliseconds), that the **Launch Manager** waits before initiating a restart attempt.
            *   **Constraint:** Must be 0 or greater.
*   **switch_run_target** (object, optional)
    *   **Description:** Defines a recovery action that switches to a different **Run Target**. This can be a new **Run Target** or the current one to retry its activation.
    *   **Properties:**
        *   **run_target** (string, optional)
            *   **Description:** Specifies the name of the **Run Target** that the **Launch Manager** should switch to upon failure.

run_target (object)
-------------------

**Description:**
  Defines a reusable type that specifies configuration parameters for a **Run Target**, outlining a particular operational mode for the system.

**Properties:**

*   **description** (string, optional)
    *   **Description:** Specifies a user-defined description of the **Run Target's** purpose or operational context.
*   **depends_on** (array of strings, optional)
    *   **Description:** Specifies the names of components and other **Run Targets** that must be successfully activated when this **Run Target** is activated. This defines the dependencies for a given operational mode.
    *   **Items:** Each item is a string specifying the name of a component or **Run Target** on which this **Run Target** depends.
*   **transition_timeout** (number, optional)
    *   **Description:** Specifies the time limit, in seconds (e.g., ``1.5`` for 1500 milliseconds), for the **Run Target** transition to complete. If this limit is exceeded, the transition is considered failed.
    *   **Constraint:** Must be greater than 0.
*   **recovery_action** (object, optional)
    *   **Description:** Specifies the recovery action to execute when a component assigned to this **Run Target** fails. This action is limited to ``switch_run_target`` operations.
    *   **Reference:** This property refers to the `recovery_action` reusable type defined in this schema, specifically enforcing the ``switch_run_target`` option.

component_properties (object)
-----------------------------

**Description:**
  Defines a reusable type that captures essential characteristics of a software component, influencing how the **Launch Manager** interacts with it.

**Properties:**

*   **binary_name** (string, optional)
    *   **Description:** Specifies the relative path of the executable file within the directory defined by ``deployment_config.bin_dir``. The final executable path will be resolved as ``{deployment_config.bin_dir}/{binary_name}``. Example values include simple filenames (e.g., ``test_app1``) or paths to executables within subdirectories (e.g., ``bin/test_app1``).
*   **application_profile** (object, optional)
    *   **Description:** Defines the application profile that specifies the runtime behavior and capabilities of this component, particularly concerning its interaction with the **Launch Manager**.
    *   **Properties:**
        *   **application_type** (string, optional)
            *   **Description:** Specifies the level of integration between the component and the **Launch Manager**.
            *   **Allowed Values:**
                *   ``"Native"``: Indicates no integration with the **Launch Manager**.
                *   ``"Reporting"``: Implies the component uses **Launch Manager** lifecycle APIs for basic reporting.
                *   ``"Reporting_And_Supervised"``: Implies the component uses lifecycle APIs and sends alive notifications to the **Launch Manager**.
                *   ``"State_Manager"``: Implies the component uses lifecycle APIs, sends alive notifications, and has permission to change the active **Run Target**.
        *   **is_self_terminating** (boolean, optional)
            *   **Description:** Indicates whether the component is designed to terminate automatically once its planned tasks are completed (``true``), or if it is expected to remain running until explicitly requested to terminate by the **Launch Manager** (``false``).
        *   **alive_supervision** (object, optional)
            *   **Description:** Defines the configuration parameters used for monitoring the "aliveness" of the component.
            *   **Reference:** This property refers to the ``alive_supervision`` reusable type defined in this schema.
            *   **Properties:** (These properties are also inherited from ``alive_supervision`` but are listed here for quick reference and clarity on the local context.)
                *   **reporting_cycle** (number, optional)
                    *   **Description:** Specifies the duration, in seconds (e.g., ``0.5`` for 500 milliseconds), of the time interval used to verify that the component sends alive notifications within the expected time frame.
                    *   **Constraint:** Must be greater than 0.
                *   **failed_cycles_tolerance** (integer, optional)
                    *   **Description:** Specifies the maximum number of consecutive reporting cycle failures. Once the number of failed cycles exceeds this maximum, the **Launch Manager** will trigger the configured recovery action.
                    *   **Constraint:** Must be 0 or greater.
                *   **min_indications** (integer, optional)
                    *   **Description:** Specifies the minimum number of checkpoints that must be reported within each configured ``reporting_cycle``.
                    *   **Constraint:** Must be 0 or greater.
                *   **max_indications** (integer, optional)
                    *   **Description:** Specifies the maximum number of checkpoints that may be reported within each configured ``reporting_cycle``.
                    *   **Constraint:** Must be 0 or greater.
*   **depends_on** (array of strings, optional)
    *   **Description:** Specifies the names of components that this component depends on. Each specified dependency must be initialized and reach its **Ready State** before the **Launch Manager** will start this component. This ensures proper startup order.
    *   **Items:** Each item is a string specifying the name of a component on which this component depends.
*   **process_arguments** (array of strings, optional)
    *   **Description:** Specifies an ordered list of command-line arguments to be passed to the component at startup.
    *   **Items:** Each item is a string specifying a single command-line argument token as a UTF-8 string; the order of arguments is preserved.
*   **ready_condition** (object, optional)
        *   **Description:** Defines the set of conditions that determine when the component completes its initializing state and enters the **Ready State**.
    *   **Properties:**
        *   **process_state** (string, optional)
            *   **Description:** Specifies the required state of the component's POSIX process for it to be considered ready.
            *   **Allowed Values:**
                *   ``"Running"``: The process has started and reached its running state.
                *   ``"Terminated"``: The process has started, reached its running state, and then terminated successfully.

deployment_config (object)
--------------------------

**Description:**
  Defines a reusable type that contains configuration parameters specific to a particular deployment environment or system setup.

**Properties:**

*   **ready_timeout** (number, optional)
    *   **Description:** Specifies the maximum time, in seconds (e.g., ``0.25`` for 250 milliseconds), allowed for the component to reach its **Ready State**. The timeout is measured from when the component's process is created until the ready conditions specified in ``component_properties.ready_condition`` are met.
    *   **Constraint:** Must be greater than 0.
*   **shutdown_timeout** (number, optional)
    *   **Description:** Specifies the maximum time, in seconds (e.g., ``0.75`` for 750 milliseconds), allowed for the component to terminate after it receives a SIGTERM signal from the **Launch Manager**. The timeout is measured from when the **Launch Manager** sends the SIGTERM signal until the operating system notifies the **Launch Manager** that the child process has terminated.
    *   **Constraint:** Must be greater than 0.
*   **environmental_variables** (object, optional)
    *   **Description:** Defines the set of environment variables passed to the component at startup.
    *   **Additional Properties:** Each key represents an environment variable name, and its value (a string) specifies the environment variable's value. An empty string is allowed and represents an intentionally empty environment variable.
*   **bin_dir** (string, optional)
    *   **Description:** Specifies the absolute filesystem path to the directory where the component's executable is installed.
*   **working_dir** (string, optional)
    *   **Description:** Specifies the directory to be used as the working directory for the component during execution.
*   **ready_recovery_action** (object, optional)
    *   **Description:** Specifies the recovery action to execute when the component fails to reach its **Ready State** within the configured ``ready_timeout``. This action is limited to ``restart`` operations.
    *   **Reference:** This property refers to the ``recovery_action`` reusable type defined in this schema, specifically enforcing the ``restart`` option.
*   **recovery_action** (object, optional)
    *   **Description:** Specifies the recovery action to execute when the component malfunctions after successfully reaching its **Ready State**. This action is limited to ``switch_run_target`` operations.
    *   **Reference:** This property refers to the ``recovery_action`` reusable type defined in this schema, specifically enforcing the ``switch_run_target`` option.
*   **sandbox** (object, optional)
    *   **Description:** Defines the sandbox configuration parameters that isolate and constrain the component's runtime execution, enhancing system security and stability.
    *   **Properties:**
        *   **uid** (integer, optional)
            *   **Description:** Specifies the POSIX user ID (UID) under which this component executes.
            *   **Constraint:** Must be 0 or greater.
        *   **gid** (integer, optional)
            *   **Description:** Specifies the primary POSIX group ID (GID) under which this component executes.
            *   **Constraint:** Must be 0 or greater.
        *   **supplementary_group_ids** (array of integers, optional)
            *   **Description:** Specifies the list of supplementary POSIX group IDs (GIDs) assigned to this component. These provide additional access permissions.
            *   **Items:** Each item is an integer specifying a single supplementary POSIX group ID (GID).
            *   **Constraint:** Each item must be 0 or greater.
        *   **security_policy** (string, optional)
            *   **Description:** Specifies the security policy or confinement profile name (such as an SELinux or AppArmor profile) assigned to the component, dictating its operating privileges.
        *   **scheduling_policy** (string, optional)
            *   **Description:** Specifies the scheduling policy applied to the component's initial thread. Supported values correspond to OS-defined policies (e.g., ``SCHED_FIFO``, ``SCHED_RR``, ``SCHED_OTHER``). Custom string values may also be supported depending on the operating system.
        *   **scheduling_priority** (integer, optional)
            *   **Description:** Specifies the scheduling priority applied to the component's initial thread, influencing its allocation of CPU time.
        *   **max_memory_usage** (integer, optional)
            *   **Description:** Specifies the maximum amount of memory, in bytes, that the component is permitted to use during runtime.
            *   **Constraint:** Must be greater than 0.
        *   **max_cpu_usage** (integer, optional)
            *   **Description:** Specifies the maximum CPU usage limit for the component, expressed as a percentage (%) of total CPU capacity.
            *   **Constraint:** Must be greater than 0.

Launch Manager Root Properties
==============================

The top-level configuration of the **Launch Manager** is structured into several distinct sections, each serving a specific purpose in defining the system's behavior and managed components. While reusable types provide the foundational building blocks, these root properties orchestrate their application to form a complete and functional configuration.

A significant portion of the configuration is dedicated to defining default behaviors and managing the primary entities:
The ``defaults`` section allows users to establish system-wide default configuration parameters that can be inherited by components and **Run Targets**, thereby reducing repetitive configurations.
The ``components`` section defines the specific software components managed by the **Launch Manager**, with each component leveraging the ``component_properties`` and ``deployment_config`` reusable types to detail its characteristics and operational environment.
The ``run_targets`` section specifies all available **Run Targets**, where each **Run Target** is an instantiation of the ``run_target`` reusable type, grouping components into operational modes.

Beyond these core sections, several other crucial properties govern the overall operation of the **Launch Manager**:
The ``schema_version`` property ensures compatibility by indicating the schema version used for the configuration file.
The ``initial_run_target`` explicitly defines which **Run Target** the **Launch Manager** must activate upon startup.
The ``fallback_run_target`` provides a specialized **Run Target** to be activated when all other recovery attempts for normal **Run Targets** have been exhausted. This particular **Run Target** does not include a ``recovery_action`` property, as it represents the ultimate state of system recovery; if its activation also fails, an external watchdog mechanism will be triggered.
The ``watchdog`` property configures the external watchdog device used for monitoring the **Launch Manager** itself.
The ``alive_supervision`` property defines the global evaluation cycle parameters for monitoring processes launched by the **Launch Manager**.

With this overview in mind, let us now examine each root configuration property in detail.

schema_version (integer, required)
----------------------------------

**Description:**
  Specifies the schema version number that the **Launch Manager** uses to determine how to parse and validate this configuration file.
**Allowed Value:**
  ``1``

defaults (object, optional)
---------------------------

**Description:**
  Defines default configuration values that components and **Run Targets** inherit unless they provide their own overriding values. Settings specified here apply globally to all components and **Run Targets**, reducing redundant configurations.

**Properties:**

*   **component_properties** (object, optional)
    *   **Description:** Defines default component property values applied to all components unless overridden in individual component definitions.
    *   **Reference:** This property refers to the ``component_properties`` reusable type defined in this schema.
*   **deployment_config** (object, optional)
    *   **Description:** Defines default deployment configuration values applied to all components unless overridden in individual component definitions.
    *   **Reference:** This property refers to the ``deployment_config`` reusable type defined in this schema.
*   **run_target** (object, optional)
    *   **Description:** Defines default **Run Target** configuration values applied to all **Run Targets** unless overridden in individual **Run Target** definitions.
    *   **Reference:** This property refers to the ``run_target`` reusable type defined in this schema.
*   **alive_supervision** (object, optional)
    *   **Description:** Defines default alive supervision configuration values. These values are used unless a global ``alive_supervision`` configuration is explicitly specified at the root level of the configuration file.
    *   **Reference:** This property refers to the ``alive_supervision`` reusable type defined in this schema.
*   **watchdog** (object, optional)
    *   **Description:** Defines default watchdog configuration values. These values are applied to all watchdogs unless overridden in individual watchdog definitions.
    *   **Reference:** This property refers to the ``watchdog`` reusable type defined in this schema.

components (object, optional)
-----------------------------

**Description:**
  Defines the software components managed by the **Launch Manager**. Each property name within this object serves as a unique identifier for a component, and its corresponding value contains the component's specific configuration.

**Pattern Properties:** Keys must be valid identifiers (alphanumeric characters, underscores, and hyphens).

*   **Component Definition** (object)
    *   **Description:** Defines an individual component's configuration properties and deployment settings.
    *   **Properties:**
        *   **description** (string, optional)
            *   **Description:** Specifies a human-readable description of the component's purpose.
        *   **component_properties** (object, optional)
            *   **Description:** Defines specific component properties for this component. Any properties not explicitly specified here will be inherited from ``defaults.component_properties``.
            *   **Reference:** This property refers to the ``component_properties`` reusable type defined in this schema.
        *   **deployment_config** (object, optional)
            *   **Description:** Defines deployment configuration for this component. Any properties not explicitly specified here will be inherited from ``defaults.deployment_config``.
            *   **Reference:** This property refers to the ``deployment_config`` reusable type defined in this schema.

run_targets (object, optional)
------------------------------

**Description:**
  Defines the **Run Targets** that represent different operational modes of the system. Each property name within this object serves as a unique identifier for a **Run Target**, and its corresponding value contains the **Run Target's** specific configuration.

**Pattern Properties:** Keys must be valid identifiers (alphanumeric characters, underscores, and hyphens).

*   **Run Target Definition** (object)
    *   **Description:** Defines an individual **Run Target's** configuration, specifying the components and dependencies that constitute a particular operational mode.
    *   **Reference:** This property refers to the ``run_target`` reusable type defined in this schema.

initial_run_target (string, required)
-------------------------------------

**Description:**
  Specifies the name of the **Run Target** that the **Launch Manager** activates during its startup sequence. This name must precisely match a **Run Target** defined within the ``run_targets`` section.

fallback_run_target (object, optional)
--------------------------------------

**Description:**
  Defines the fallback **Run Target** configuration. The **Launch Manager** activates this **Run Target** when all recovery attempts for other **Run Targets** have been exhausted. This specific **Run Target** does not include a ``recovery_action`` property, as it represents the final state; if its activation also fails, the external watchdog will be triggered.

**Properties:**

*   **description** (string, optional)
    *   **Description:** Specifies a human-readable description of the fallback **Run Target**.
*   **depends_on** (array of strings, required)
    *   **Description:** Specifies the names of components and **Run Targets** that must be activated when this fallback **Run Target** is activated.
    *   **Items:** Each item is a string specifying the name of a component or **Run Target** upon which this **Run Target** depends.
*   **transition_timeout** (number, optional)
    *   **Description:** Specifies the time limit, in seconds (e.g., ``1.5`` for 1500 milliseconds), for the **Run Target** transition. If this limit is exceeded, the transition is considered failed.
    *   **Constraint:** Must be greater than 0.

alive_supervision (object, optional)
------------------------------------

**Description:**
  Defines the global alive supervision configuration parameters used to monitor component health. If specified, this configuration will override any default values set in ``defaults.alive_supervision``.
**Reference:** This property refers to the ``alive_supervision`` reusable type defined in this schema.

watchdog (object, optional)
---------------------------

**Description:**
  Defines the global external watchdog device configuration used by the **Launch Manager**. If specified, this configuration will override any default values set in ``defaults.watchdog``.
**Reference:** This property refers to the ``watchdog`` reusable type defined in this schema.

Default Values
**************

The **Launch Manager** configuration extensively utilizes the concept of default values to streamline the configuration process and simplify initial setup. The fundamental principle is that even if a specific configuration value is not explicitly provided by the user, a default value will always be applied, ensuring a complete and valid configuration.

To achieve this robust defaulting mechanism, the **Launch Manager** employs two distinct levels of default values:

1.  **User-Defined Defaults:** These are specified within the ``defaults`` section of the **Launch Manager** configuration. Users can define a set of configuration options here, which will be applied if those options are not provided at a more specific level within the configuration (e.g., within an individual component, a **Run Target**, or at the root level for properties like ``alive_supervision`` or ``watchdog``).
2.  **S-CORE Standard Defaults:** These represent a second tier of default values, provided by the S-CORE standard itself. The purpose of these defaults is to ensure a valid configuration value is available even if an option is entirely absent from both its specific definition and the user-defined ``defaults`` section.

The S-CORE standard defaults are particularly beneficial during the development phase. They allow developers to concentrate on core tasks without needing to meticulously define every configuration option, as a functional default is guaranteed.

S-CORE Standard Defaults
========================

The **Launch Manager** guarantees the availability of these specific default values, which are applied when no corresponding configuration is found in the explicit definition of a property (e.g., within a component's ``deployment_config``, a **Run Target**'s settings, or a root-level property) or within the user-defined ``defaults`` section.

alive_supervision
-----------------

The S-CORE standard provides the following default values for ``alive_supervision`` properties if they are not explicitly defined elsewhere:

.. code-block:: json

   {
      "evaluation_cycle": 0.5
   }

watchdog
--------

The S-CORE standard provides the following default values for ``watchdog`` properties if they are not explicitly defined elsewhere:

.. code-block:: json

   {
      "device_file_path": "/dev/watchdog",
      "max_timeout": 2.0,
      "deactivate_on_shutdown": true,
      "require_magic_close": false
   }

run_target
----------

The S-CORE standard provides the following default values for ``run_target`` properties if they are not explicitly defined elsewhere:

.. code-block:: json

   {
      "description": "",
      "depends_on": [],
      "transition_timeout": 3,
      "recovery_action": {
         "switch_run_target": {
            "run_target": "Off"
         }
      }
   }

component_properties
--------------------

The S-CORE standard provides the following default values for ``component_properties`` if they are not explicitly defined elsewhere:

.. code-block:: json

   {
      "binary_name": "",
      "application_profile": {
         "application_type": "Reporting_And_Supervised",
         "is_self_terminating": false,
         "alive_supervision": {
            "reporting_cycle": 0.5,
            "failed_cycles_tolerance": 2,
            "min_indications": 1,
            "max_indications": 3
         }
      },
      "depends_on": [],
      "process_arguments": [],
      "ready_condition": {
         "process_state": "Running"
      }
   }

deployment_config
-----------------

The S-CORE standard provides the following default values for ``deployment_config`` if they are not explicitly defined elsewhere:

.. code-block:: json

   {
      "ready_timeout": 0.5,
      "shutdown_timeout": 0.5,
      "environmental_variables": {},
      "bin_dir": "/opt",
      "working_dir": "/tmp",
      "ready_recovery_action": {
         "restart": {
            "number_of_attempts": 0,
            "delay_before_restart": 0
         }
      },
      "recovery_action": {
         "switch_run_target": {
            "run_target": "Off"
         }
      },
      "sandbox": {
         "uid": 1000,
         "gid": 1000,
         "supplementary_group_ids": [],
         "security_policy": "",
         "scheduling_policy": "SCHED_OTHER",
         "scheduling_priority": 0,
         "max_memory_usage": 0,
         "max_cpu_usage": 0
      }
   }

Inheritance Of Default Values
=============================

Given that the **Launch Manager** supports multiple levels of default values, specific rules govern their inheritance and application. The inheritance order is straightforward:

1.  If a configuration value is not explicitly specified at a specific location (e.g., within an individual component's definition, a **Run Target**'s definition, or a root-level property like ``alive_supervision``), the **Launch Manager** will first attempt to use the corresponding value from the user-defined ``defaults`` section.
2.  If the value is also not specified within the user-defined ``defaults`` section, then the **Launch Manager** will apply the S-CORE standard default value for that option.
