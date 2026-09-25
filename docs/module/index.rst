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

Lifecycle Module
################

.. mod:: Lifecycle Module
   :id: mod__lifecycle
   :version: 1
   :security: YES
   :safety: ASIL_B
   :status: valid
   :includes: comp__health_monitor[version==1], comp__lifecycle_launch_manager[version==1]


Module View
-----------

.. mod_view_sta:: Module architecture
   :id: mod_view_sta__lifecycle__all
   :version: 1
   :includes: comp__lifecycle_launch_manager, comp__health_monitor
   :belongs_to: mod__lifecycle

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_module(need(), needs) }}
      LifecycleApplication --> logic_arc_int__lifecycle__lifecycle_if : implements
      LifecycleApplication --> logic_arc_int__lifecycle__controlif : use
      LifecycleApplication --> logic_arc_int__lifecycle__alive_if : use
      LifecycleApplication --> logic_arc_int__lifecycle__logical_monitor_if : use
      LifecycleApplication --> logic_arc_int__lifecycle__deadline_monitor_if :use
      LifecycleApplication --> posix_signals : implements
      NativeApplication --> posix_signals : implements
      comp__lifecycle_launch_manager --> posix_signals : use



Module Documents
----------------

.. toctree::
   :maxdepth: 1

   manuals/index
   safety_mgt/index
   security_mgt/index
