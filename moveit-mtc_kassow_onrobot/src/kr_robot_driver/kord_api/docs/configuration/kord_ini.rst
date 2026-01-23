.. _kord_ini:

=======================
KORD Configuration File
=======================

The `KORD.ini` file is essential for configuring the KORD application. This file must be imported into the **workcell** before execution. It defines various parameters that govern the behavior and communication between the client PC and the robot's CBun.

Contents:
---------

- `Prerequisites <#prerequisites>`_

  - `1. Upload KORD.ini to the Teach Pendant <#upload-kord-ini>`_
  - `2. Add SSH Key to Your PC <#add-ssh-key>`_

- `Configuration File Structure <#configuration-file-structure>`_

  - `Section [KORD] <#kord>`_
  - `Section [QOC_MONITOR] <#qoc-monitor>`_
  - `Section [QOC_HALT_TRIGGER] <#qoc-halt-trigger>`_
  - `Section [IO] <#io>`_
  - `Section [MOTION_CONSTRAINTS] <#motion-constraints>`_

- `Example Configuration <#example-configuration>`_
- `Handling Statistics Issues <#handling-statistics-issues>`_

Prerequisites
=============

Before transferring data using the `KORD.ini` file, ensure the following prerequisites are met:

1. **Upload KORD.ini to the Teach Pendant**
2. **Add SSH Key to Your PC**

.. _upload-kord-ini:

1. Upload KORD.ini to the Teach Pendant
---------------------------------------

The `KORD.ini` file provides essential information about the client PC (currently supported for **Linux** and **macOS** machines). This file must be uploaded to the Teach Pendant to establish communication between the client and the robot.
In order to upload the `KORD.ini` file to the Teach Pendant, follow these steps:

1. Upload KORD.ini to a USB flask drive
2. Insert the USB drive into the controller
3. On Teach Pendant, go to `Workcell > Files > Add file` and select the configuration file.
4. KORD.ini will be displayed in the `Files` section of the `Workcell` tab.

**Mandatory Fields:**

.. code-block:: ini
    :caption: Minimal `KORD.ini` with Mandatory Fields

    [KORD]
    username = my_username_on_pc

This configuration is generally sufficient for KORD to operate. However, additional parameters can be specified to enhance functionality.

**Optional Fields:**

.. code-block:: ini
    :caption: `KORD.ini` with Optional Parameters

    [KORD]
    username = my_username_on_pc
    calibration_path = ~/kassow_calibration/        ; Path for calibration data
    diagnostics_path = ~/kassow_diagnostics/      ; Path for diagnostics data
    log_path = ~/kassow_logs/                      ; Path for logs data
    host = 192.168.X.Y                              ; Host IP address
    show_dialogs = yes                              ; Set to "no" to disable dialogs (default: "yes")

**Parameter Descriptions:**

- **username**: *(Mandatory)* Username of the client PC for file transfers using SCP.
- **calibration_path**: *(Optional)* Directory path on the client PC for transferring calibration data.
- **diagnostics_path**: *(Optional)* Directory path on the client PC for transferring diagnostics data.
- **log_path**: *(Optional)* Directory path on the client PC for transferring log files.
- **host**: *(Optional)* IP address of the client PC for establishing connections.
- **show_dialogs**: *(Optional)* Enables or disables dialogs on the Teach Pendant. Set to `"no"` to disable.

.. _add-ssh-key:

2. Add SSH Key to Your PC
-------------------------

To facilitate secure file transfers between the Teach Pendant and your PC, an SSH key must be added to the PC's `authorized_keys`.

**Steps to Add SSH Key:**

.. code-block:: bash
    :caption: Commands to Add SSH Key

    export pub_key="ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQCadKOmGzamjx1+FMmMCweVIDCb7NQmYSwnKfeqO9f+StmWvcLZNAYkB5A7UoiuRG+pLzAXAdQCKptEryXdfCvR3BcMAXBJHfAZiAVTesEf5cStZB4KREgE9HOidR4Y1mSheZuVZEFxREQijk6okbad31jdMQRGrjofjBodENDbcRDPdKB8vyi8Cdoq22R5phBV1m0lTsV9kNSO2/74Xsb3i6bTw/fCZhlEW+SDvJpzcizHP+Kt/CWD8jAHDNaVHQdTAgF5QVvlwjxUgBeuM5u5+ZDfDb2QPAS1bhspvQ1IdVCjQOLypDb8Ql2Huh0FNkPdcvUHePIs3qwv5If9O913 nucky@r2d2"
    echo $pub_key >> ~/.ssh/authorized_keys

**Explanation:**

- **Export `pub_key`**: Defines the public SSH key.
- **Append to `authorized_keys`**: Adds the public key to the list of authorized keys on the PC, enabling password-less SSH authentication.

These steps are essential for transferring files such as the URDF file of calibration data securely.

Configuration File Structure
============================

The `KORD.ini` file is divided into several sections, each responsible for different aspects of the KORD application's behavior and communication. Below is a detailed breakdown of each section and its parameters.

.. _kord:

Section [KORD]
--------------

General setup and paths for transferring files. Relative paths are also valid.

+--------------------+-----------------------------------------------------------------------------------------+
| **Field**          | **Description**                                                                         |
+====================+=========================================================================================+
| `username`         | *(Mandatory)* Username of the client PC for file transfers using SCP.                   |
+--------------------+-----------------------------------------------------------------------------------------+
| `log_path`         | *(Optional)* Directory on the client PC where logs are transferred.                     |
+--------------------+-----------------------------------------------------------------------------------------+
| `diagnostics_path` | *(Optional)* Directory on the client PC where diagnostics data are transferred.         |
+--------------------+-----------------------------------------------------------------------------------------+
| `calibration_path` | *(Optional)* Directory on the client PC where calibration files are transferred.        |
+--------------------+-----------------------------------------------------------------------------------------+
| `host`             | *(Optional)* IP address of the client PC for establishing connections.                  |
+--------------------+-----------------------------------------------------------------------------------------+
| `show_dialogs`     | *(Optional)* Set to `"no"` to disable dialogs on the Teach Pendant. Defaults to `"yes"`.|
+--------------------+-----------------------------------------------------------------------------------------+

.. _qoc-monitor:

Section [QOC_MONITOR]
---------------------

Statistics parameters captured and processed on the **KORD CBun** side only. For configuring API statistics parameters, use relevant API calls as detailed in the :ref:`API Statistics <api-statistics>` section.

+---------------------+-------------------------------------------------------------------------------------+
| **Field**           | **Description**                                                                     |
+=====================+=====================================================================================+
| `stat_window_size`  | Window size in seconds for recent statistics (e.g., `0.2` seconds).                 |
+---------------------+-------------------------------------------------------------------------------------+

.. _qoc-halt-trigger:

Section [QOC_HALT_TRIGGER]
--------------------------

Detailed statistics parameters to trigger safety events. If thresholds are exceeded, a safety event is triggered to stop robot movement.

+-------------------------------+-----------------------------------------------------------------------------------------------------+
| **Field**                     | **Description**                                                                                     |
+===============================+=====================================================================================================+
| `enabled`                     | Enables (`yes`) or disables (`no`) the QOC_HALT_TRIGGER feature.                                    |
+-------------------------------+-----------------------------------------------------------------------------------------------------+
| `on_consecutive_commands_lost`| Number of consecutive lost frames to trigger a soft stop (e.g., `2`).                               |
+-------------------------------+-----------------------------------------------------------------------------------------------------+
| `on_recent_commands_lost`     | Number of command requests lost in the recent window period to trigger a soft stop (e.g., `5`).     |
+-------------------------------+-----------------------------------------------------------------------------------------------------+
| `on_recent_avg_system_jitter` | Average system jitter threshold in microseconds (e.g., `100`).                                      |
+-------------------------------+-----------------------------------------------------------------------------------------------------+
| `on_recent_max_system_jitter` | Maximum system jitter threshold in microseconds (e.g., `500`).                                      |
+-------------------------------+-----------------------------------------------------------------------------------------------------+
| `on_recent_avg_roundtrip_time`| Average roundtrip time threshold in microseconds (e.g., `300`).                                     |
+-------------------------------+-----------------------------------------------------------------------------------------------------+
| `on_recent_avg_cmd_jitter`    | Average command jitter threshold in microseconds (e.g., `500`).                                     |
+-------------------------------+-----------------------------------------------------------------------------------------------------+
| `on_movej_cmd_ceased_span`    | Threshold for MoveJ command ceased span in radians (e.g., `1e-6`). Prevents abrupt changes.         |
+-------------------------------+-----------------------------------------------------------------------------------------------------+
| `on_movel_cmd_ceased_span`    | Threshold for MoveL command ceased span in meters (e.g., `1e-6`). Prevents abrupt changes.          |
+-------------------------------+-----------------------------------------------------------------------------------------------------+

.. _io:

Section [IO]
------------

Configure safe digital outputs. Each output can have one of the following values:

- `0` or `1` : **Disabled**
- `2` : **Enabled**
- `3` : **PStop Mapping**
- `4` : **EStop Mapping**
- `5` : **PStop and EStop Mapping**

+-----------+--------------------------------------+
| **Field** | **Description**                      |
+===========+======================================+
| `SDO1`    | Safe Digital Output 1 (Default: `0`) |
+-----------+--------------------------------------+
| `SDO2`    | Safe Digital Output 2 (Default: `0`) |
+-----------+--------------------------------------+
| `SDO3`    | Safe Digital Output 3 (Default: `0`) |
+-----------+--------------------------------------+
| `SDO4`    | Safe Digital Output 4 (Default: `0`) |
+-----------+--------------------------------------+

**Example Configuration:**

.. code-block:: ini
    :caption: [IO] Section Example

    [IO]
    SDO1 = 0 ; Disabled
    SDO2 = 5 ; PStop and EStop Mapping
    SDO3 = 3 ; PStop Mapping
    SDO4 = 0 ; Disabled

.. _motion-constraints:

Section [MOTION_CONSTRAINTS]
----------------------------

Set motion constraints for the robot, including joint speeds, accelerations, and workspace parameters.

**Joint Space Constraints:**

+---------------------+--------------------------------------------+
| **Field**           | **Description**                            |
+=====================+============================================+
| `joint1_speed`      | Joint 1 speed in degrees per second (deg/s)|
+---------------------+--------------------------------------------+
| `joint2_speed`      | Joint 2 speed in degrees per second (deg/s)|
+---------------------+--------------------------------------------+
| `joint3_speed`      | Joint 3 speed in degrees per second (deg/s)|
+---------------------+--------------------------------------------+
| `joint4_speed`      | Joint 4 speed in degrees per second (deg/s)|
+---------------------+--------------------------------------------+
| `joint5_speed`      | Joint 5 speed in degrees per second (deg/s)|
+---------------------+--------------------------------------------+
| `joint6_speed`      | Joint 6 speed in degrees per second (deg/s)|
+---------------------+--------------------------------------------+
| `joint7_speed`      | Joint 7 speed in degrees per second (deg/s)|
+---------------------+--------------------------------------------+

**Joint Acceleration Constraints:**

+-------------------------+------------------------------------------+
| **Field**               | **Description**                          |
+=========================+==========================================+
| `joint1_acceleration`   | Joint 1 acceleration in deg/s²           |
+-------------------------+------------------------------------------+
| `joint2_acceleration`   | Joint 2 acceleration in deg/s²           |
+-------------------------+------------------------------------------+
| `joint3_acceleration`   | Joint 3 acceleration in deg/s²           |
+-------------------------+------------------------------------------+
| `joint4_acceleration`   | Joint 4 acceleration in deg/s²           |
+-------------------------+------------------------------------------+
| `joint5_acceleration`   | Joint 5 acceleration in deg/s²           |
+-------------------------+------------------------------------------+
| `joint6_acceleration`   | Joint 6 acceleration in deg/s²           |
+-------------------------+------------------------------------------+
| `joint7_acceleration`   | Joint 7 acceleration in deg/s²           |
+-------------------------+------------------------------------------+

**Workspace Constraints:**

+---------------------+-----------------------------------------------------------+
| **Field**           | **Description**                                           |
+=====================+===========================================================+
| `ws_speed`          | Workspace speed in meters per second (m/s)                |
+---------------------+-----------------------------------------------------------+
| `ws_acceleration`   | Workspace acceleration in meters per second squared (m/s²)|
+---------------------+-----------------------------------------------------------+

**Example Configuration:**

.. code-block:: ini
    :caption: [MOTION_CONSTRAINTS] Section Example

    [MOTION_CONSTRAINTS]
    ; Joint space - deg/s
    joint1_speed = 225.0
    joint2_speed = 225.0
    joint3_speed = 225.0
    joint4_speed = 225.0
    joint5_speed = 225.0
    joint6_speed = 225.0
    joint7_speed = 225.0

    ; Joint space - deg/s2
    joint1_acceleration = 10000.0
    joint2_acceleration = 10000.0
    joint3_acceleration = 10000.0
    joint4_acceleration = 10000.0
    joint5_acceleration = 10000.0
    joint6_acceleration = 10000.0
    joint7_acceleration = 10000.0

    ; Workspace - m/s
    ws_speed = 2.0

    ; Workspace - m/s2
    ws_acceleration = 25.0

Example Configuration
=====================

Below is an example of a complete `KORD.ini` file incorporating all sections and parameters:

.. code-block:: ini
    :caption: Complete `KORD.ini` Example

    [KORD]
    username = user
    log_path = ~/kassow_logs/
    diagnostics_path = ~/kassow_diagnostics/
    calibration_path = ~/kassow_calibration/
    host = 192.168.100.193
    show_dialogs = yes

    [QOC_MONITOR]
    ; If the following parameters are exceeded, a safety event is triggered to stop robot movement
    stat_window_size = 0.2 ; Window size in seconds for recent statistics

    [QOC_HALT_TRIGGER]
    enabled = yes ; Enable/disable the halt trigger
    on_consecutive_commands_lost = 2 ; Number of consecutive lost frames to trigger a soft stop
    on_recent_commands_lost = 5 ; Number of command requests lost in the recent window period
    on_recent_avg_system_jitter = 100 ; Average system jitter threshold in microseconds
    on_recent_max_system_jitter = 500 ; Maximum system jitter threshold in microseconds
    on_recent_avg_roundtrip_time = 300 ; Average roundtrip time threshold in microseconds
    on_recent_avg_cmd_jitter = 500 ; Average command jitter threshold in microseconds
    on_movej_cmd_ceased_span = 1e-6 ; Threshold for MoveJ command ceased span in radians
    on_movel_cmd_ceased_span = 1e-6 ; Threshold for MoveL command ceased span in meters

    [IO]
    SDO1 = 0 ; Disabled
    SDO2 = 5 ; PStop and EStop Mapping
    SDO3 = 3 ; PStop Mapping
    SDO4 = 0 ; Disabled

    [MOTION_CONSTRAINTS]
    ; Joint space - deg/s
    joint1_speed = 225.0
    joint2_speed = 225.0
    joint3_speed = 225.0
    joint4_speed = 225.0
    joint5_speed = 225.0
    joint6_speed = 225.0
    joint7_speed = 225.0

    ; Joint space - deg/s²
    joint1_acceleration = 10000.0
    joint2_acceleration = 10000.0
    joint3_acceleration = 10000.0
    joint4_acceleration = 10000.0
    joint5_acceleration = 10000.0
    joint6_acceleration = 10000.0
    joint7_acceleration = 10000.0

    ; Workspace - m/s
    ws_speed = 2.0

    ; Workspace - m/s²
    ws_acceleration = 25.0


Handling System Issues
======================

The KORD CBun continuously monitors these statistics to ensure safe, efficient
robot operation. If thresholds configured in the ``QOC_HALT_TRIGGER`` are exceeded:

1. **Notification:** A pop-up or alarm dialog appears on the Teach Pendant,
   describing the issue.
2. **Safety Trigger:** Robot movement is automatically suspended if
   ``QOC_HALT_TRIGGER.enabled = yes`` and any threshold is violated.

To resume operation, clear the CBun error state and any other active alarms.

**Additional Information:**

- Detailed descriptions of system jitter, roundtrip time, command jitter, and other statistics can be found in the :doc:`../statistics/statistics` section.

.. warning::
    **Safety First:** Always ensure that safety parameters are correctly configured to prevent unintended robot movements that could lead to accidents or equipment damage.

