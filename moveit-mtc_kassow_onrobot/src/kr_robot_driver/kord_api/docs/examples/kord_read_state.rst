
.. _ex-read-state:

Read State
==========

.. note::
    New in v2.0.0 (beta) - reading of the pose with quaternion is now supported.

The robot controller software **motion** flags, **safety** flags can be read. 
Safety modes are also provided via the receive API.

To read the motion flags the control session needs to be initiated.

.. code-block:: c++

    // Sync MUST pass before any data can be fetched.
    kord->syncRC();

    // Loop continuously syncing and updating status data
    while (true) {
        
        // Sync with the KORD frequency
        if (!kord->waitSync(std::chrono::milliseconds(10))){
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }

        // Retrieve recently received data
        rcv_iface.fetchData();

        // Read state Information
        unsigned int safety_flags = rcv_iface.getRobotSafetyFlags(); // Read the safety flags
        unsigned int motion_flags = rcv_iface.getMotionFlags();
        unsigned int safety_mode = rcv_iface.getSafetyMode();
        double master_speed = rcv_iface.getMasterSpeed();

        // Complete alarm state, returns 0 when the system is not suspended or containing any alarm state
        uint32_t alarm_state = rcv_iface.systemAlarmState();
    
        // Review its values and evaluate them...
    }

IO state
--------

It is possible to read digital IO status as the following:

.. code-block:: c++

    unsigned int digital_input = rcv_iface.getDigitalInput();   // Get the digital IO Input status
    unsigned int digital_output = rcv_iface.getDigitalOutput(); // Get the digital IO Output status

However, it might be convenient for certain cases call helper function as the following:

.. code-block:: c++

    std::cout << rcv_iface.getFormattedInputBits()  << std::endl;
    std::cout << rcv_iface.getFormattedOutputBits() << std::endl;


See :ref:`API documentation <flags>` to find out about the flags meaning.