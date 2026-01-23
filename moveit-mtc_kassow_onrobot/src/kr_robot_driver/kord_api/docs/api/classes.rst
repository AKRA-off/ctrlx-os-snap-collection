Classes 
-------

:cpp:class:`kr2::kord::KordCore` is the main class of the KORD API, which provides access to the main functionality of the API.
It is responsible for creating and managing the :cpp:class:`kr2::kord::ReceiverInterface` and :cpp:class:`kr2::kord::ControlInterface` objects, and providing access to the API configuration.

kord::KordCore
~~~~~~~~~~~~~~
.. doxygenclass:: kr2::kord::KordCore 
  :members:

.. doxygenenum:: kr2::kord::EFrameID

.. doxygenenum:: kr2::kord::EFrameValue

.. doxygenenum:: kr2::kord::ELoadID

.. doxygenenum:: kr2::kord::ELoadValue

kord::ReceiverInterface
~~~~~~~~~~~~~~~~~~~~~~~
.. doxygenclass:: kr2::kord::ReceiverInterface 
  :members:


kord::ControlInterface
~~~~~~~~~~~~~~~~~~~~~~
.. doxygenclass:: kr2::kord::ControlInterface
   :members:

.. doxygenenum:: kr2::kord::TrackingType

.. doxygenenum:: kr2::kord::BlendType

.. doxygenenum:: kr2::kord::OverlayType

.. doxygenenum:: kr2::kord::ControlInterface::EClearRequest
