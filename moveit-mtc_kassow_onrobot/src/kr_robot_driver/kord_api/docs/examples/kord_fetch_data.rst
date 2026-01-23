Fetch kinematic calibration data
================================

Usage
-----

.. note::
    The provided example requires the robot to operate in tabletless mode, meaning the tablet should not be connected to the robot.

.. code-block::

 $ ./kord_fetch_data -c 192.168.0.1
    [OPTIONS]
     -h               show this help
     -r <prio>        execute as a realtime process with priority set to <prio>
     -p <port>        port number to connect to
     -c <IP Address>  remote controller IP address
     -i <Session ID>  KORD session ID | Default: 1


Description
-----------

The provided example skips data fetching that occurs when either physical robot arm or robot model was changed.

The full example can be found in ``examples/kord_skip_fetch_data.cpp``.

.. code-block:: c++

    // Create and transmit a system communication request
    kord::RequestSystem request;
    request.asServerCommunication(true);
    ctl_iface.transmitRequest(request);

    // ...

   // HW_STAT_INIT_RUID_MISMATCH indicates the need of fetching calibration data
   if (rcv_iface.getHWFlags() & HW_STAT_INIT_RUID_MISMATCH) {
       auto parameters = std::make_shared<ServiceFetchKincalParameters>(true);
       auto req = kord::RequestServer().asServiceRequest(EServerServiceCommands::eStart,
                                                         EKORDServerServiceID::eFetchKincalData,
                                                         parameters);

       if (!kord->waitSync(std::chrono::milliseconds(100))) {
           KORD_LOG_ERROR("Sync wait timed out, exiting.");
       }
       rcv_iface.fetchData();

       ctl_iface.transmitRequest(req);
       KORD_LOG_INFO("Kincal fetch start was sent successfully");
       // Sleep to prevent the request from being overridden
       std::this_thread::sleep_for(std::chrono::seconds(3));
   }

   // ...

   // Create and transmit a status request
   auto status_req = kord::RequestServer().asServiceRequest(EServerServiceCommands::eGetStatus, EKORDServerServiceID::eFetchKincalData);
   auto token = ctl_iface.transmitRequest(status_req);

   while (!stop) {
       if (!kord->waitSync(std::chrono::milliseconds(100))) {
           KORD_LOG_ERROR("Sync wait timed out, exiting.");
           break;
       }

       // ...

       rcv_iface.fetchData();
       if (rcv_iface.hasResponse(token)) {
           auto response = rcv_iface.getResponse<kord::protocol::ServerResponse>(token);
           uint status = response.getStatus();
           uint progress = response.getProgress();

           KORD_LOG_INFO("Response - progress: " << progress << ", status: " << status);

           if (status == static_cast<uint8_t>(EServiceStatus::eSuccess) ||
               status == static_cast<uint8_t>(EServiceStatus::eFailed) ||
               status == static_cast<uint8_t>(EServiceStatus::eIdle)) {
               stop = true;
           }

           // Transmit the status request again
           token = ctl_iface.transmitRequest(status_req);
       }
   }

    // Stop the service by transmitting a stop request
    auto stop_req = kord::RequestServer().asServiceRequest(EServerServiceCommands::eStop, EKORDServerServiceID::eFetchKincalData);
    ctl_iface.transmitRequest(stop_req);

    if (!kord->waitSync(std::chrono::milliseconds(100))) {
        KORD_LOG_ERROR("Sync wait timed out, exiting.");
        return EXIT_FAILURE;
    }

    // Disable server communication
    request.asServerCommunication(false);
    ctl_iface.transmitRequest(request);
