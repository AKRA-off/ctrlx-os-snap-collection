Set Safe Digital Output
=======================

The KORD API allows to set safe digital outputs to 5 different persistent configurations:

- Disabled (``ESafePortConfiguration::eSafePortDisabled``).
- Enabled (``ESafePortConfiguration::eSafePortEnabled``).
- PStop mapped (``ESafePortConfiguration::eSafePortPStopMapped``).
- EStop mapped (``ESafePortConfiguration::eSafePortEStopMapped``).
- P+EStops mapped (``ESafePortConfiguration::eSafePortBothMapped``).

Description
-----------

The provided example suppose to enable:

 * SDO1 with P+EStops mapped.

In order to disable certain digital outputs, the users are suggested to use the
``ESafePortConfiguration::eSafePortDisabled`` argument.
The full example can be found in ``examples/kord_set_safe_output.cpp``.

.. code-block:: c++

    // the enums are defined in the proper header. Here they are attached for clarity.
    enum DIGITAL_SAFE { SDO1 = MASK_SDO1, SDO2 = MASK_SDO2, SDO3 = MASK_SDO3, SDO4 = MASK_SDO4 };

    // Create a request to the remote controller
    kr2::kord::RequestIO io_request;
    io_request.asSetIODigitalOut().withEnabledSafePorts(kr2::kord::RequestIO::DIGITAL_SAFE::SDO1,
                                                        ESafePortConfiguration::eSafePortBothMapped);

    kord->sendCommand(io_request);
    KORD_LOG_INFO("TX Request    RID: " << io_request.request_rid_);
