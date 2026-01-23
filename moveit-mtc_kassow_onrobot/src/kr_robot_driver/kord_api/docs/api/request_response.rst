
.. _responses:

Request/Response Channel
========================

.. note:: Request/Response channel is available in v2.0.0 and later.
          Please note that it is **not** designed for real-time communication.

At this moment, the following requests are available:

- Get robot information (robot model, Tool IO status, serial numbers).
- Get robot safety zones.
- Get SW, HW and FW versions.
- Server requests.

Using the Request/Response Channel
----------------------------------

The Request/Response channel in the KORD API enables you to send specific requests to the robot controller and handle the corresponding responses.
This channel is ideal for non-real-time operations such as fetching system information, retrieving safety zones, and managing server services.

Below is a step-by-step guideline on how to utilize the Request/Response channel effectively, illustrated with references to the provided examples.

Step 1: Create the Appropriate Request
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Construct the desired request using the appropriate request type from the `kord::RequestSystem` or `kord::RequestServer` classes.

.. code-block:: cpp

    // GetVersion Request
    auto request = kord::RequestSystem().asGetVersion();

    // GetSafetyZones Request
    auto request = kord::RequestSystem().asGetSafetyZones();

    // Server Service Request
    auto request = kord::RequestServer.asServiceRequest(EServerServiceCommands::eStart, EKORDServerServiceID::eFetchKincalData, parameters);


Step 2: Transmit the request
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use the :cpp:class:`kr2::kord::ControlInterface` to transmit the request to the robot controller.
The return value is a token that you can use to identify the response.

.. code-block:: cpp

    // Transmit the request
    auto token = ctl_iface.transmitRequest(request);

Step 3: Wait for the Response and Handle it
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

After transmitting a request, you need to wait for the corresponding response from the robot controller.
This involves checking if the response is available and then retrieving it using the `getResponse` method.

Once the response is available, use the templated `getResponse()` method to retrieve the response data.
The template parameter should be the response type, e.g., :cpp:class:`kr2::kord::protocol::GetRobotInfoResponse`, :cpp:class:`kr2::kord::protocol::GetSafetyZonesResponse`, :cpp:class:`kr2::kord::protocol::GetVersionResponse`, etc.

.. code-block:: cpp

    while (!stop)
        if (!kord->waitSync(100ms)) {
            KORD_LOG_ERROR("Sync failed");
            break;
        }
        rcv_iface.fetchData();

        // Wait for the response
        if (rcv_iface.hasResponse(token)) {
            // T is the response type, e.g., GetRobotInfoResponse, GetSafetyZonesResponse, GetVersionResponse, etc.
            auto response = rcv_iface.getResponse<T>(token);
            // Process the response
            KORD_LOG_INFO(response);
            break;
        }
    }


API reference
-------------

kord::protocol::GetRobotInfoResponse
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. doxygenclass:: kr2::kord::protocol::GetRobotInfoResponse
    :project: kord-api
    :members:
    :undoc-members:


kord::protocol::GetSafetyZonesResponse
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. doxygenclass:: kr2::kord::protocol::GetSafetyZonesResponse
    :project: kord-api
    :members:
    :undoc-members:

kord::protocol::GetVersionResponse
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. doxygenclass:: kr2::kord::protocol::GetVersionResponse
    :project: kord-api
    :members:
    :undoc-members:

kord::protocol::ServerResponse
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. doxygenclass:: kr2::kord::protocol::ServerResponse
    :project: kord-api
    :members:
    :undoc-members: