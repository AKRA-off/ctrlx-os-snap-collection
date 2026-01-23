Linear Movement
===============

Running the Example
-------------------

The robot will move in the z axis down approximately 30 cm and up to the original 
pose in a slow pace. Allow enough space under the TCP to avoid collisions.

.. note::
 To assure the robot does not arrive at its position limits, please move the robot into ``[0 60 0 60 0 60 0]``.

.. code-block:: none

  $ ./kord_move_linear -h
    [OPTIONS]
     -h               show this help
     -r <prio>        execute as a realtime process with priority set to <prio>
     -p <port>        port number to connect to
     -c <IP Address>  remote controller IP address
     -i <Session ID>  KORD session ID | Default: 1

To make the robot move run: 

.. code-block:: bash

 $ ./kord_move_linear -p 7582 -c 192.168.0.1


Description
-----------
The full example can be found in ``examples/kord_linear_movement.cpp``.

To move the robot in linear space the TCP references you need to provide
the targets with every update. Send the target after every heartbeat message 
will be captured.

.. code-block:: c++

    // Establish the control session

    while(g_run) {


        // Update tcp target
        tcp_target[0] = start_tcp[0];
        tcp_target[1] = start_tcp[1];
        tcp_target[2] = (std::cos(t * 2e-4)-1)*a + start_tcp[2];
        tcp_target[3] = start_tcp[3];
        tcp_target[4] = start_tcp[4];
        tcp_target[5] = start_tcp[5];
        t = i * 7;
        i++;
        
        //kord->spin();
        if (!kord->waitSync(std::chrono::milliseconds(10))){
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }

        ctl_iface.moveL(tcp_target, 0.008, 0.004);
    }

