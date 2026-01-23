Joint Movement
==============

Running the example
-------------------

In order to run the example it is advised to position the robot into the following pose: ``[0 60 0 60 0 60 0]``.

.. code-block:: none

  $ ./kord_move_joints
    [OPTIONS]
     -h               show this help
     -r <prio>        execute as a realtime process with priority set to <prio>
     -p <port>        port number to connect to
     -c <IP Address>  remote controller IP address
     -i <Session ID>  KORD session ID | Default: 1

Compile the example and then run the following command ``./kord_move_joints -p 7582 -c 192.168.0.1``.

.. code-block:: none

  $ ./kord_move_joints -t 10 -p 7582 -c 192.168.0.1
  Connecting to: 192.168.0.1:7582
  [KORD-API] Session ID: 1
  Connection successful.
  n: 1446
  KORD Payload length: 1430
  Sync Captured 
  Read initial joint configuration:
  5.33984e-12 60.0306 -7.45632e-09 60.0306 5.33984e-12 60.0306 6.72484e-12 
  Robot stopped
  Digital Inputs  [16|DI| 1]: 0000 0000 0000 0000
  Digital Outputs [PSU: 2 1] [4 |TB| 1] [8 |B| 1] [4 |R| 1]: 00 0000 0000 0000 0000
  SSTOP; PSTOP, ESTOP
  false, false, false
  Safety flags: 1
  Runtime: 9.996 [s]
  SafetyFlags: 1
  MotionFlags: 2
  
  
  FailEmpty: 0
  FailError: 26
  RCState: 1
                         MinDelay[ms]           MaxDelay[ms]           AverageDelay[ms]       
  Jitter                 0.176                  0.519                  0.382                  
  Age                    3.772                  4.254                  3.999                  
  Api                    3.77877                4.24353                3.99847                
  Failed to receive: 0


In case you use either the RT patched kernel or a low latency kernel you may 
execute the movement as a real time process.

.. code-block:: bash

 $ sudo ./kord_move_joints -p 7582 -c 192.168.0.1 -r 30

Description
-----------

The full example can be found in ``examples/kord_move_joints.cpp``.

To move the joints you need to provide the target references. The target
must be transmitted every 4ms. Correct timing is derived from the capture 
of the heartbeat ``ArmStatus`` frame.

.. code-block:: c++

    // Establish the control session...

    while(g_run) {
        // Update q

        q[0] = (std::cos(t * 2e-4)-1)*a + start_q[0];
        q[1] = (std::cos(t * 3.3e-4)-1)*a + start_q[1];
        q[2] = (std::cos(t * 4.5e-4)-1)*a + start_q[2];
        q[3] = (std::cos(t * 2.4e-4)-1)*a + start_q[3];
        q[4] = (std::cos(t * 6e-4)-1)*a + start_q[4];
        q[5] = (std::cos(t * 8e-4)-1)*a + start_q[5];
        q[6] = (std::cos(t * 1e-3)-1)*a + start_q[6];
        t = i * 7;
        i++;

        if (!kord->waitSync(std::chrono::milliseconds(10))){
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }

        ctl_iface.moveJ(q, kr2::kord::TrackingType::TT_TIME, 0.008,
                        kr2::kord::BlendType::BT_TIME, 0.004);
    }

.. note::
    To achieve the proper timing, the command
    ``ctl_iface.moveJ(q, kr2::kord::TrackingType::TT_TIME, 0.008, kr2::kord::BlendType::BT_TIME, 0.004)``
    needs to be send as the first command after the ``waitSync()``.