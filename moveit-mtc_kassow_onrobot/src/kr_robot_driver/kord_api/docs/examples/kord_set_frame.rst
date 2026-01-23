
.. _ex-set_frame:

Set Frames values
=================

It is possible to set different frames using theo following method:

.. code-block:: c++

    // Sync MUST pass before any data can be fetched.
    kord->syncRC();
    rcv_iface.fetchData();

    // Set TCP in reference to TFC
    std::array<double, 6UL> p = {1, 4, 2, 0, 0, 0};
    int64_t token; // token of the command to check its status
    ctl_iface.setFrame(kord::EFrameID::TCP_FRAME, p, kord::EFrameValue::POSE_VAL_REF_TFC, token);

    // Waiting for the command status-result
    while(rcv_iface.getCommandStatus(token) == -1){
        if (!kord->waitSync(std::chrono::milliseconds(20))){
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }
        rcv_iface.fetchData(); // Always fetching to see new values
    }

    // Review its values and evaluate them...
