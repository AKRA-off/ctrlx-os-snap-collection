
.. _ex-set_load:

Set Loads values
================

It is possible to set different loads using functions:

.. code-block:: c++

    // Sync MUST pass before any data can be fetched.
    kord->syncRC();
    rcv_iface.fetchData();

    double new_mass = 12.3;
    std::array<double, 3UL> p = {0.3, 0.7, 0}; // Preparing CoG
    std::array<double, 6UL> in = {0, 0, 0, 0.4 ,0.5 ,0.6}; // Inertia

    int64_t token; // Token of the command to check its status
    ctl_iface.setLoad(kord::ELoadID::LOAD2, new_mass, p, in, token);
    
    // Waiting for the command status result
    while(rcv_iface.getCommandStatus(token) == -1){
        if (!kord->waitSync(std::chrono::milliseconds(20))){
            KORD_LOG_ERROR("Sync wait timed out, exit");
            break;
        }
        rcv_iface.fetchData(); // always fetching to see new values
    }

    // Review its values and evaluate them...
