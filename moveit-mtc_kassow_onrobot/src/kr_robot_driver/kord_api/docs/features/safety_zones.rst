.. _safety_zones:

==============
Safety Zones
==============

Safety zones define the operational limits of the robot's movements to ensure safe and efficient performance.

.. important::
  KORD API only supports reading safety zones. Modifying safety zones is not supported.

Overview
----------------

Safety zones are virtual boundaries that:

- Restrict robot movement to predefined operational areas.
- Use geometric primitives with 3D coordinates and normals to define the shape and orientation of the zone.
- Enforce speed limits and safety buffers.

KORD API introduces several limitations on the number of safety zones and the size of the labels:

- Maximum 12 user-defined safety zones can be retrieved.
- Labels will be truncated to 128 characters, if longer.

.. note::
  The robot can have more safety zones, but only 12 can be retrieved using the KORD API.

Safety Zones Reading
--------------------

Safety zones can be retrieved from the KORD API by sending the appropriate request and parsing the response.

.. code-block:: cpp

  auto request = kord::RequestSystem().asGetSafetyZones();
  auto token = ctl_iface.transmitRequest(request);

  while (!stop) {
      if (!kord->waitSync(1s)) {
          KORD_LOG_ERROR("Sync failed");
          break;
      }
      rcv_iface.fetchData();
      if (rcv_iface.hasResponse(token)) {
          auto response = rcv_iface.getResponse<kord::protocol::GetSafetyZonesResponse>(token);
          // Parse the response using provided methods
      }
  }

Please refer to the :cpp:class:`kr2::kord::protocol::GetSafetyZonesResponse` documentation for more information on the response structure.

Default Safety Zones
--------------------

.. note::
  Torque safety mode (SM.Torque) is system-managed and not user-configurable.

Robots maintain these fundamental safety modes through predefined zones:

- Safe speed mode:

  - Turtle, DUID 1000, SM.Safe.
  - Low speed limits.

- Reduced speed mode:

  - Rabbit, DUID 1001, SM.Reduced.
  - Reduced speed limits.
  
- Unlimited speed mode:

  - Cheetah, DUID 1002, SM.Normal.
  - Full operational speeds.

- Torque safety mode:

  - Internal, DUID 1003, SM.Torque.
  - Automatically engaged during backdrive.
