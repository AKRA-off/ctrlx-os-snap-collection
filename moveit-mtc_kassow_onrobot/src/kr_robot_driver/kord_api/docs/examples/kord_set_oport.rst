Set Digital IO Output
=====================

The KORD-API has the possibility to set digital outputs for the Relay, IO Board, and Tool Board.

Quick start
-----------

.. code-block:: none

  $ ./kord_set_oport -p 7582 -c 192.168.0.1

Description
-----------

The provided example suppose to enable:

 * Digital relay ouput 1;
 * Digital board outputs 6 and 8;
 * Tool board output 1 with default 24V configuration.

In order to disable certain digital outputs, the users are suggested to use the ``.withDisabledPorts()`` keyword.
The full example can be found in ``examples/kord_set_oport.cpp``.

.. code-block:: c++

    // the enums are defined in the proper header. Here they are attached for clarity.
    enum DIGITAL_RELAYS  { RELAY1=MASK_RELAY1, RELAY2=MASK_RELAY2, RELAY3=MASK_RELAY3, RELAY4=MASK_RELAY4 };
    enum DIGITAL_IOBOARD { DO1=MASK_DO1, DO2=MASK_DO2, DO3=MASK_DO3, DO4=MASK_DO4,
                           DO5=MASK_DO5, DO6=MASK_DO6, DO7=MASK_DO7, DO8=MASK_DO8 };
    enum DIGITAL_IOTOOLB { TB1=MASK_TB1, TB2=MASK_TB2, TB3=MASK_TB3, TB4=MASK_TB4 };

    // Create a request to the remote controller
    kr2::kord::RequestIO io_request;
    io_request.asSetIODigitalOut()
              .withEnabledPorts( // Enable Relay 1, Digital Output 8, Digital Output 6 and Tool Board 1 [24V]
                kr2::kord::RequestIO::DIGITAL_RELAYS::RELAY1 |
                kr2::kord::RequestIO::DIGITAL_IOBOARD::DO8 |
                kr2::kord::RequestIO::DIGITAL_IOBOARD::DO6 |
                kr2::kord::RequestIO::DIGITAL_IOTOOLB::TB1);
    
    kord->sendCommand(io_request);
    KORD_LOG_INFO("TX Request    RID: " << io_request.request_rid_);
