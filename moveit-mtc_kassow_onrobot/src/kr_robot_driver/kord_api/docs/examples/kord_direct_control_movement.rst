Direct Control Movement
=======================

Running the example
-------------------

.. warning::
    In order to successfully run the direct joint control mode a appropriate version of responsive
    controller is needed. The direct joint control feature is available the alpha release of the
    EraElektrix revision 4 .

In order to run the example it is advised to position the robot into the following pose: ``[0 60 0 60 0 60 0]``.

.. code-block:: none

 $ ./kord_move_direct [OPTIONS]
    Options:
    -a, --amplitude <value>   Amplitude in radians for every joint (default: 1.6)
    -f, --period <value>      Move period in seconds (default: 4.0)
    -h, --help                Display this help message

Compile the example and then run the following command ``./kord_move_direct -c 192.168.0.1 -a 1.6 -f 4.0``
to connect to the controller at ``192.168.0.1:7582`` and move the robot with an amplitude of 1.6 radians and a period of 4 seconds.

.. code-block:: none

  $ ./kord_move_direct -p 7582 -c 192.168.0.1 -a 1.6 -f 4.0
    Connecting to: 192.168.0.1:7582
    [KORD-API] Session ID: 1
    Connection successful.
    KORD Payload length: 1430
    Sync Captured 
    Read initial joint configuration:
    -85.9896 55.2367 -158.241 67.0405 -119.273 -86.8302 -112.401 
    Robot stopped
    Digital Inputs  [16|DI| 1]: 0000 0000 0000 0000
    Digital Outputs [PSU: 2 1] [4 |TB| 1] [8 |B| 1] [4 |R| 1]: 00 0000 0000 0000 0000
    SSTOP; PSTOP, ESTOP
    false, false, false
    Safety flags: 1
    Runtime: 10 [s]
    SafetyFlags: 1
    MotionFlags: 0

    FailEmpty: 0
    FailError: 30
    RCState: 1
                        MinDelay[ms]           MaxDelay[ms]           AverageDelay[ms]       
    Jitter                 0.188                  0.603                  0.397                  
    Age                    3.378                  4.65                   3.999                  
    Api                    3.42058                4.44886                3.99843                
    Failed to receive: 0


In case you use either the RT patched kernel or a low latency kernel you may 
execute the movement as a real time process.

Description
-----------

The full example can be found in ``examples/kord_move_direct.cpp``.

To move the joints you need to provide the target references. In this example,
we provide reference joint positions (q), reference joint velocities (qd), and
reference joint accelerations (qdd).
It is possible to provide torque as well, but it will be computed automatically in case of zeros, 
in order to fulfill the required velocities and accelerations.
The target must be transmitted every 4ms. Correct timing is derived from the capture 
of the heartbeat which is detected by using the ``waitSync()`` function.

.. code-block:: c++

    // Establish the control session...

    while(g_run) {
        // update q, qv(or 'q dot'), qa(or 'q dot dot')

        for (size_t i = 0; i<7; i++) 
        {
            double b = i * 1.0e-3;
            q  [i]    =  a*std::cos(t * b) - a + start_q[i];
            qd [i]    = -a*b*std::sin(t * b);
            qdd[i]    = -a*b*b*std::cos(t * b);
            torque[i] = 0.0; // it will be computed automatically in case of zero
        }
        t = i * 7;
        i++;

        if (!kord->waitSync(std::chrono::milliseconds(10))){
            KORD_LOG_ERORR("Sync wait timed out, exit");
            break;
        }

        ctl_iface.directJControl(q, qd, qdd, torque);
    }

.. note::
    To achieve the proper timing, the command ``ctl_iface.directJControl(q, qd, qdd, torque);`` 
    needs to be send as the first command after the ``waitSync()``.