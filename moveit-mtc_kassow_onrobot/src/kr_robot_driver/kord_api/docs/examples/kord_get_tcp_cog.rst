
.. _kord_get_tcp_cog:

Get TCP and Frames
==================

The recent TCP position (reference) coordinates with regards to the World Frame can be read as following:

.. code-block:: c++

    // Sync MUST pass before any data can be fetched.
    kord->syncRC();
    rcv_iface.fetchData();

    // Read the recent TCP coordinates expressed with regards to the World Frame
    fetched_TCP = rcv_iface.getTCP();

    // Read the recent CoG of the Load2 (Tool)
    fetched_CoG = rcv_iface.getCoG();
    
    // Review its values and evaluate them...

Get Frame and Load
-------------------

Complete frame setup of the TCP or Loads (1, 2) can be read by using the getFrame() and getLoads() methods. 

.. code-block:: c++

    std::vector<std::variant<double, int>> pose = rcv_iface.getFrame(kord::EFrameID::TCP_FRAME, kord::EFrameValue::POSE_VAL_REF_TFC);   // Get the Frame values
    std::vector<std::variant<double, int>> pose = rcv_iface.getLoad(kord::ELoadID::LOAD1, kord::ELoadValue::INERTIA_VAL);   // Get the Load Inertia

These methods are updating relevant information in the interactive fashion, which means the longer period of time is needed
to extract the whole data set. For that reason methods shouldn't be used in the real-time dependent processing.

.. code-block:: c++

    // Sync MUST pass before any new data data can be fetched.
    kord->syncRC(0); // 0 option will ignore the completion of data rotation
    rcv_iface.fetchData();

    // Use to perform multiple fetches in a row to update all cycle of data, takes 40ms
    if (!kord->syncRC()){
        KORD_LOG_ERROR("Sync RC failed.")";
        return EXIT_FAILURE;
    }
    // or
    if (!kord->waitSync(std::chrono::milliseconds(10), 1)){
        KORD_LOG_ERROR("Sync RC failed.");
        return EXIT_FAILURE;
    }

