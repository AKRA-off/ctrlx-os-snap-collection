
.. _ex-read-temperature:

Reading Temperatures
====================

With the latest Kassow Robots RC releases it is possible to read onboard CPU temperatures by invoking the `getCPUState()` method.

.. code-block:: c++

    double tempIO_q = rcv_iface.getIOBoardTemperature();

    std::cout << "IOBoard Temperature: " << tempIO_q;
    std::cout << std::endl;

    std::cout << "CPU Temperatures" << '\n';
    //average temperature
    std::cout << "Pack 0: " << rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::PACKAGE_ID0_TEMP) << std::endl; 

    std::cout << "Core 0: " << rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::CORE_0_TEMP) << std::endl;
    std::cout << "Core 1: " << rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::CORE_1_TEMP) << std::endl;
    std::cout << "Core 2: " << rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::CORE_2_TEMP) << std::endl;
    std::cout << "Pack 3: " << rcv_iface.getCPUState(kord::ReceiverInterface::ECPUStateValue::CORE_3_TEMP) << std::endl;


Please note the CPU state information is updated at low frequency. Using the `syncRC()` with the empty list of parameters will provide the complete update of recent values in a blocking fashion.

.. code-block:: c++

    // Sync MUST pass before any new data data can be fetched.
    kord->syncRC(0); // with the param 0, the api provides only a single communication sync
    rcv_iface.fetchData();

    // syncRC() with no parameters performs all necessary data fetches to update all cycle of data, takes ~40ms
    if (!kord->syncRC()){
        std::cout << "Sync RC failed.\n";
        return EXIT_FAILURE;
    }

    // Using F_SYNC_FULL_ROTATION param will request the regular data update to run through all cyclic data to be updated
    if (!kord->waitSync(std::chrono::milliseconds(10), kord::F_SYNC_FULL_ROTATION)){
        std::cout << "Sync RC failed.\n";
        return EXIT_FAILURE;
    }

