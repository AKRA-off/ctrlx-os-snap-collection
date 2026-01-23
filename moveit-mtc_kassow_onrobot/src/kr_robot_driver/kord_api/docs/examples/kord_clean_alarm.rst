
.. _ex-clean_alarm:

Clear/Recover RC state
======================

Usage
-----

.. code-block:: none

 $ ./kord_clean_alarm -h
    [OPTIONS]
        -h               show this help
        -r <prio>        execute as a realtime process with priority set to <prio>
        -p <port>        port number to connect to
        -c <IP Address>  remote controller IP address
        -i <Session ID>  KORD session ID | Default: 1
    [DEDICATED OPTIONS]
        --all                            Clear all errors
        --halt                           Clear halt on the controller
        --unsuspend                      Unsuspend the robot
        --init                           Continue robot initialization if blocked
        --cbun                           Acknowledge CBun error and clear it


The example clears alarm specified by the arguments (dedicated ``--halt``, ``--unsuspend``, ``--init``, ``--cbun`` arguments).
The type of alarm to clean can be get using `systemAlarmState()` in ``./kord_read_state``. For example, to clean the CBun Error do:

.. code-block:: bash

 $ ./kord_clean_alarm -c 192.168.0.1 --cbun


Description
-----------

With the clear alarm state method `clearAlarmRequest()` it is possible to recover the internal software state suspending the robot from regular operations:

.. code-block::

    CLEAR_HALT - Clear halt state (software state residue of PSTOP, ESTOP, SSTOP actions)
    UNSUSPEND - Clear the suspended state invoked by the previous user interaction (toggle button, pause buttong)
    RESUME - Software state invoked by the stop command in automatic mode
    CONTINUE_INIT - Request to continue robot initialisation after it was interrupted (joints locked)
    CBUN_EVENT - Clear CBunEvent Error, this error usually originates when the statistics exceeding set limits.

.. code-block:: c++

    // Sync MUST pass before any data can be fetched.
    kord->syncRC();
    rcv_iface.fetchData();

    // Unsuspending the robot
    int64_t token = ctl_iface.clearAlarmRequest(kord::ControlInterface::EClearRequest::UNSUSPEND);

    // Or

    // Clearing Halt State
    int64_t token = ctl_iface.clearAlarmRequest(kord::ControlInterface::EClearRequest::CLEAR_HALT);
    
    // Waiting for the command status result
    while(rcv_iface.getCommandStatus(token) == -1) {
        if (!kord->waitSync(std::chrono::milliseconds(20))) {
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }
        rcv_iface.fetchData(); // always fetching to see new values
    }

    // Review its values and evaluate them...
