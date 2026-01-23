Read Events
===========

System Error Transferring
-------------------------

This example shows how to track the current state of the system.

The handled states are **StopEvents** and **SafetyEvents**.

Usage
-----

.. code-block:: none

 $ ./kord_retrieve_errors -h
    [OPTIONS]
        -h               show this help
        -r <prio>        execute as a realtime process with priority set to <prio>
        -p <port>        port number to connect to
        -c <IP Address>  remote controller IP address
        -i <Session ID>  KORD session ID | Default: 1


Compile the example and then run the following command ``./kord_retrieve_errors -p 7582 -c 192.168.0.1``
The command will track the current state of the robot.
Typical output is the following:

.. code-block:: bash

    ...
    Read the last system event: 
    ...
    [PSTOP BUTTON PRESSED]
    Read the last system event: 
        [Timestamp|1e+00]:1667493164442079488.00 [ID]:2001 [Event Group]:1
    ...

    ... Example with 2 events:
    [Timestamp|1e+00]:1667493164442079488.00 [ID]:2001 [Event Group]:1
    [Timestamp|1e-9]:10 [ID]:1010 [Event Group]:2


The timestamp is a time stamp from the RC. 
Additional events would be printed with respect to the previous event. 

Meaning of ``ID`` and ``Event Group`` are the following:

Event Groups
------------

There are a limited number of event groups. See :ref:`API Event groups <events-group>` for more details.


Event Indices
-------------

There are a limited number of event identifier based on the group they belong to.
See :ref:`API Safety Events <events-group-safety-event>`  and  
:ref:`API Soft Stop Events <events-group-soft-stop-event>` for more details.

