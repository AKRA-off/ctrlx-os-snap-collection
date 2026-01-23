.. _introduction-description:

====================
Introduction
====================

KORD operates on a **client-server** architecture, where:

- **Client (API):** A device intended to control the robot (e.g., a laptop).
- **Server (CBun):** Executed on the robot side, handling communication and control commands.

.. note::
    **CBun is available for customers only.**

Connecting to the Robot
=======================

For optimal performance and reliability, it is recommended to establish a **direct wired connection** between the controlling device (client) and the robot. Follow these guidelines to ensure a successful connection:

1. **Network Configuration:**

   - **Static IP Addresses:** Assign static IP addresses to both the client and the robot to avoid connectivity issues.
   - **Same Network Segment:** Ensure both devices are within the same network segment to facilitate seamless communication.

2. **Default IP Settings:**

   - **Robot (CBun) Default IP:** `192.168.39.1`
   - **Recommended Client IP:** For example, `192.168.39.5`

   It is advisable to keep the robot's IP address unchanged and configure the client accordingly.

.. warning::
    **Avoid using USB to Ethernet adapters** if possible, as they may introduce latency or connectivity issues.

Initiating a Control Session
============================

To begin controlling the robot using KORD, you must initiate a control session. This involves synchronizing the client with the robot's CBun to ensure coordinated command transmission.

**Steps to Initiate a Control Session:**

1. **Initialize Communication:**

   The client establishes a connection with the robot's CBun by calling the :cpp:func:`kr2::kord::KordCore::connect` method.

2. **Synchronize with the Robot:**

   The CBun expects the client to synchronize with it via the :cpp:func:`kr2::kord::KordCore::syncRC` method. The client calls the :cpp:func:`kr2::kord::KordCore::syncRC` method to indicate readiness to receive control commands.

3. **Capture and Transmit Status:**

   Upon receiving the initialization request, the CBun captures it and begins transmitting Robot Status Heart Beats (RSHB) to the remote controller at regular intervals of **4ms**. The RSHB transmission is crucial for synchronizing the client/controller, marking the precise moments for sending control commands.

.. note::
    - The RSHB is transmitted every **4ms**, aligning with the controller's update loop.
    - Only **one controller** can be connected at a time, and RSHB are sent exclusively to the initiating device.

.. code-block:: cpp
    :caption: Example Code to Initiate Control Session

    // Create an instance of KordCore for handling RX/TX KORD frames.
    std::shared_ptr<kord::KordCore> kord(new kord::KordCore("192.168.39.1", 7582, 1, kord::UDP_CLIENT));

    // Connect establishes the socket and initializes defaults.
    // An internal timer is initiated to match the robot's control loop frequency.
    if (!kord->connect()) {
        std::cout << "Connecting to KR failed\n";
        return EXIT_FAILURE;
    }

    // SyncRC sends an ArmStatus request to the robot's CBun to initiate control session.
    // Upon success, status data can be fetched via the receive interface,
    // and commands can be sent via the control interface.
    if (!kord->syncRC()){
        std::cout << "Sync RC failed.\n";
        return EXIT_FAILURE;
    }

Getting Started
===============

This section provides a brief overview of setting up both the **KORD CBun** and the **KORD API** to begin controlling your robot.

KORD CBun
---------

The **CBun** serves as the server component on the robot side. It requires specific configurations to handle incoming connections effectively.

**Configuration Parameters:**

- **Port Number:** Specify the port number on which CBun listens for incoming connections.
- **Real-Time Priority:** Assign real-time priority to ensure timely processing of control commands.
- **RT Disable Switch:** An option to disable real-time operations, intended for testing and debugging purposes only.

**Setup Steps:**

1. **Provide Port Number:** Configure CBun with the desired port number for listening to incoming client connections.
2. **Enable Real-Time Priority:** Activate real-time priority to enhance performance and reduce latency.
3. **(Optional) Disable RT:** Use the RT disable switch if real-time operations need to be turned off for testing or debugging.

KORD API
--------

Refer to the :ref:`Installation Guide <installation-description>` for detailed instructions on obtaining, compiling, and installing the KORD API.

Minimal Example
===============

The following C++ example demonstrates how to initiate a control session, retrieve joint positions, print them to the command line, and exit.

**Prerequisites:**

- The robot's IP address is set to `192.168.39.1`.
- KORD CBun is activated on the robot.
- Real-time mode is **not** required for retrieving data.

.. code-block:: cpp
    :caption: Minimal Example to Retrieve and Print Joint Positions

    #include <kord/api/kord_control_interface.h>
    #include <kord/api/kord_receive_interface.h>

    using namespace kr2;

    int main()
    {
        // Create an instance of KordCore for handling RX/TX KORD frames.
        std::shared_ptr<kord::KordCore> kord(new kord::KordCore("192.168.39.1", 7582, 1, kord::UDP_CLIENT));

        // Initialize Control and Receiver Interfaces.
        kord::ControlInterface ctl_iface(kord);
        kord::ReceiverInterface rcv_iface(kord);

        // Establish connection to the robot's CBun.
        if (!kord->connect()) {
            std::cout << "Connecting to KR failed\n";
            return EXIT_FAILURE;
        }

        // Send ArmStatus request to initiate control session.
        if (!kord->syncRC()){
            std::cout << "Sync RC failed.\n";
            return EXIT_FAILURE;
        }

        std::cout << "Sync Captured \n";

        // Fetch and retrieve joint positions.
        rcv_iface.fetchData();
        std::array<double, 7UL> start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);

        // Print initial joint configuration in degrees.
        std::cout << "Read initial joint configuration:\n";
        for(double angle : start_q)
            std::cout << (angle / 3.14) * 180 << " ";
        std::cout << "\n";

        return 0;
    }


**Understanding the Example:**

- :cpp:class:`kr2::kord::KordCore`: Manages the low-level communication between the client and the CBun.
- :cpp:class:`kr2::kord::ControlInterface`: Handles sending control commands to the robot.
- :cpp:class:`kr2::kord::ReceiverInterface`: Manages receiving data from the robot, such as joint positions.

.. warning::
    **Real-Time Mode:** While the example does not run in real-time mode, enabling real-time operations can enhance synchronization and performance for more complex applications.

