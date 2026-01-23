Brake Control
=============

The full example can be found in ``examples/kord_brake_control.cpp``.

It is possible to unlock one joint only or a set of joints. The locking
works the same way.

Usage
-----

.. code-block:: none

 $ ./kord_brake_control -h
   [OPTIONS]
       -h               show this help
       -r <prio>        execute as a realtime process with priority set to <prio>
       -p <port>        port number to connect to
       -c <IP Address>  remote controller IP address
       -i <Session ID>  KORD session ID | Default: 1
   [DEDICATED OPTIONS]
       --engage          <1,2,3,4,5,6,7>        Lock the specified joints, locks all joints if none provided
       --disengage       <1,2,3,4,5,6,7>        Unlock the specified joints, unlocks all joints if none provided

The example engages or disengages the brakes (based on ``--engage`` or ``--disengage`` arguments) of the joints 5, 6, 7. For example, to engage the brakes execute:

.. code-block:: bash

 $ ./kord_brake_control -c 192.168.0.1 --engage # specify joints in the code
 $ # or
 $ ./kord_brake_control -c 192.168.0.1 --engage --engage=5,6,7 # specify via launch arguments

.. warning::
    Do not use the lock and unlock commands at the same time. The 
    controller can currently handle only one group of commands 
    at a time. Restrict yourself to either locking or unlocking 
    the joints.

Description
-----------

To control the brakes use:

.. code-block:: c++

 bool ControlInterface::engageBrakes(const std::vector<int> &a_joints)
 bool ControlInterface::disengageBrakes(const std::vector<int> &a_joints)

The commands will be sent out directly. That is the reason why currently 
it is not advised to transmit both control commands in one update.
Locking and unlocking brakes take time and are power greedy. The controller
tries to avoid it by not unlocking all brakes at once, but rather 
in a sequence. To get the status of operations,
overload ``(dis)engageBrakes(const std::vector<int> &a_joints, int64_t& out_token)``
in a combination with ``rcv_iface.getCommandStatus(token)`` can be used.
Don't forget to perform waitSync() to fetch new states. 

.. note::
    The joints do not provide feed back about their state. It can be that
    the joints did not unlock successfully. 

.. code-block:: c++

    // Establish control session
    // ...
    // Wait for the heartbeat
    if (!kord->waitSync(std::chrono::milliseconds(10))){
        KORD_LOG_ERROR("Sync wait timed out, exit");
        break;
    }

    // When heartbeat was captured, transmit the request - brake joints 5,6,7
    std::vector<int> joints{5,6,7};
    ctl_iface.engageBrakes(joints);
    // ctl_iface.disengageBrakes(joints);