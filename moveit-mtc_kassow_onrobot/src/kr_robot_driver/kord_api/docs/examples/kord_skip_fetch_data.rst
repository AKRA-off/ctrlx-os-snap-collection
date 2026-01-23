Skip kinematic calibration fetch
================================

Usage
-----

.. code-block::

 $ ./kord_skip_fetch_data -c 192.168.0.1
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

   // HW_STAT_INIT_RUID_MISMATCH indicates the need of fetching calibration data
   if (rcv_iface.getHWFlags() & HW_STAT_INIT_RUID_MISMATCH) {
        KORD_LOG_INFO("In fetch data state, sending the command...");
        if (!kord->waitSync(std::chrono::milliseconds(100))){
            KORD_LOG_ERROR("Sync wait timed out, exit");
        }
        // Create and send a requests to skip calibration data fetching
        kr2::kord::RequestRCAPICommand sys_request;
        sys_request.asUserConsent().addPayload(kr2::kord::protocol::ERCAPIPayloadCmdConsentId::eSkipFetch);
        ctl_iface.transmitRequest(sys_request);
    }
