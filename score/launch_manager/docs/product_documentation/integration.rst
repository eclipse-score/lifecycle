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

.. _lm_integration:

Integration
***********

Configuration
=============

The **Launch Manager** is currently configured with a single json file.
See :ref:`lm_config` and :ref:`lm_config_examples` for details about the **Launch Manager** configuration format.

.. _lm_integration_generating_flatbuffer_file:

Generating flatbuffer file
--------------------------

The json configuration has to be converted to a flatbuffer file before it can
be used by the **Launch Manager**.

The provided bazel function ``launch_manager_config`` handles the compilation
of the json configuration to flatbuffer files.

.. code-block:: python

   load("@score_lifecycle//:defs.bzl", "launch_manager_config")

   # This is your launch manager json configuration
   exports_files(["lm_config.json"])

   # Afterwards, you can refer to the generated flatbuffer files with :example_config_gen
   # The generated flatbuffer file has identical as the json configuration file, but with ".bin" file ending (example: "lm_config.bin")
   launch_manager_config(
       name ="example_config_gen",
       config="//scripts/config_mapping:lm_config.json"
   )

.. _lm_integration_location_of_configuration_file:

Location of the configuration file
----------------------------------

The **Launch Manager** can be started with a CLI flag defining the absolute or
relative path to the configuration file:

``launch_manager -c <config path>``

Example: ``launch_manager -c /opt/launch_manager/etc/lm_config.bin``

If ``-c`` is not specified, the **Launch Manager** will attempt to load the
configuration from the default path ``etc/launch_manager_config.bin``.

State manager mw::com config
----------------------------

An example ``mw::com`` config is provided with the following target
``//score/launch_manager:example_com_config``.
This is a single config that includes both **Launch Manager** and the
**State Manager** instance definition so they can be used by both.


For more information about the ``mw::com`` configuraiton please refer
to their documentation and tutorial at documentaiton
https://eclipse-score.github.io/communication/v0.5.0/score/mw/com/doc/tutorial/chapter_2/README.html#separate-configuration-files-for-provider-and-consumer

LmControlService service type
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. dropdown:: LmControlService service type

   .. literalinclude:: examples/mw_com_config.json
      :language: json

Service types
"""""""""""""

* **serviceTypeName**

  ``.serviceTypes[].serviceTypeName: "/score/mw/lifecycle/LmControlService"`` - 
  This is the name of the **Launch Manager** service.

* **version**

  ``.serviceTypes[].version: { "major": 1, "minor": 0 }`` - 
  This is the version of the service interface.

* **binding**

  ``.serviceTypes[].bindings[].binding: "SHM"`` - 
  This is the communication mechanism.

  ``.serviceTypes[].bindings[].serviceId: 7101`` - 
  This is the numeric identifier of the service for this binding.
  Note this can be changed to any number, however has to be unique to any
  other service used by this application.

* **events**

  ``.serviceTypes[].bindings[].events[]`` - 
  Events offered by the service. Each event has a name and a numeric id.
  Note the ID can be changed to any number, however has to be unique to any
  other binding used by this application.

  .. code-block:: json

    {
      "eventName": "ActivationResult",
      "eventId": 1
    }

* **methods**

  ``.serviceTypes[].bindings[].methods[]`` - 
  Methods offered by the service. Each method has a name and a numeric id.
  Note the ID can be changed to any number, however has to be unique to any
  other binding used by this application.

  .. code-block:: json

    {
      "methodName": "ActivateRunTarget",
      "methodId": 2
    },
    {
      "methodName": "GetActiveRunTarget",
      "methodId": 3
    }

Service instances
"""""""""""""""""

The ``serviceInstances`` array defines the concrete instances of a service type.
Wwo instances of ``LmControlService`` are required, one used by the
**Launch Manager** (``LaunchManager/StateManager/Instance``) and one used by the
**State Manager** (``StateManager/LaunchManager/Instance``).

* **instanceSpecifier**

  ``.serviceInstances[].instanceSpecifier: "LaunchManager/StateManager/Instance"`` - 
  This is the name an application uses to look up the instance.

* **serviceTypeName**

  ``.serviceInstances[].serviceTypeName: "/score/mw/lifecycle/LmControlService"`` - 
  This is the service type the instance is created from. It must match a
  ``serviceTypeName`` in ``serviceTypes``.

* **version**

  ``.serviceInstances[].version: { "major": 1, "minor": 0 }``  -
  This is the version of the service type and must match the one in ``serviceTypes``.

* **instances**

  ``.serviceInstances[].instances[].instanceId`` - 
  Numeric identifier of the instance.
  Note the ID can be changed to any number, however has to be unique to any
  other instance used by this application.

  ``.serviceInstances[].instances[].asil-level`` - 
  Safety integrity level of the instance, Either ``QM`` or ``ASIL-B``.

  ``.serviceInstances[].instances[].binding`` - 
  Communication mechanism, shall be ``SHM``.

  ``.serviceInstances[].instances[].events[]`` - 
  Per-method settings, for each method ``methodName``.

  ``.serviceInstances[].instances[].numberOfSampleSlots`` - 
  Number of sample slots allocated for the event.
  **This is only configured on the server instance** i.e.
  ``LaunchManager/StateManager/Instance``.

  ``.serviceInstances[].instances[].maxSubscribers`` - 
  Maximum number of simultaneous subscribers.
  **This is only configured on the server instance** i.e.
  ``LaunchManager/StateManager/Instance``.

  ``.serviceInstances[].instances[].methods[]`` - 
  Per-method settings, for each method ``methodName``.

  ``.serviceInstances[].instances[].queueSize`` - 
  Number of calls that can be queued for the method.

  .. code-block:: json

    {
      "instanceId": 1,
      "asil-level": "QM",
      "binding": "SHM",
      "events": [
        {
          "eventName": "ActivationResult",
          "numberOfSampleSlots": 8,
          "maxSubscribers": 1
        }
      ],
      "methods": [
        {
          "methodName": "ActivateRunTarget",
          "queueSize": 1
        }
      ]
    }

* **global**

  ``.global.asil-level: "QM"``

  This is the default safety integrity level for the whole configuration.

Implementation
==============

Starting a process
------------------

A reporting process must signal to the Launch Manager that it has
started successfully by calling the lifecycle API for the language it is
implemented in.

A ``ready_timeout_ms`` can be configured for each component.
If the process does not report ``Running`` within that timeout, the Launch
Manager terminates the process and registers an error.

Stopping a process
------------------

A process can be configured to terminate in 2 ways:

-  **Self-terminating** - The process terminates whenever it wants.
-  **Non-self-terminating** - The process remains alive until LCM requests the process to end using a ``SIGTERM`` signal.

For a process to support being terminated by LCM, it needs to respond to
a ``SIGTERM`` signal by ending ongoing tasks, freeing resources and
exiting.
Note that a **Self-terminating** also recieves the ``SIGTERM`` signal when LCM
decides to deactivate it.

If a process does not terminate after receiving a ``SIGTERM``, and a
``shutdown_timeout_ms`` is configured, then after the timeout is fired a
``SIGKILL`` is sent and the process is forcefully terminated.

Examples
--------

.. dropdown:: C Reporting Process

   .. literalinclude:: examples/c_reporting_process.c
      :language: c

.. dropdown:: C++ Reporting Process

   .. literalinclude:: examples/cpp_reporting_process.cpp
      :language: cpp

.. dropdown:: Rust Reporting Process

   .. literalinclude:: examples/rust_reporting_process.rs
      :language: rust


The **Lifecycle Interface** also offers the ``run_application`` API, which lets
a user application delegate its initialization specific code to a replaceable
class.
This means that the user application can run under any initialization system
without being rebuilt.

.. dropdown:: C++ Lifecycle Application

   .. literalinclude:: examples/cpp_lifecycle_application.cpp
      :language: cpp


State management
----------------

A **State Manager** is an application that decides which run-targets shall be
active by communicating with the Launch Manager using the ``ILmControl``
interface.
The **State Manager** is itself a managed component.
Before calling ``activate_run_target``, it must report the Running state to the
Launch Manager via the **Lifecycle API**.
Otherwise, the Launch Manager does not consider the State
Manager ready, which can lead to errors.

Additionally, the State Manager component must have
``application_profile.application_type`` set to ``State_Manager``.

Example
^^^^^^^

.. dropdown:: State Manager example

   .. literalinclude:: examples/example_state_manager.cpp
      :language: cpp
