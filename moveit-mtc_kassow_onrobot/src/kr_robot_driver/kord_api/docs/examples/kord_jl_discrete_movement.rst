Discrete Joint/Linear Movements
===============================

Apart from continuous movement (e.g `Direct Control Movement`), the KORD provides the possibility to use Discrete Movement too. 
This feature is enriching a variety of movement control by lowering requirements for `sending` device.

Quick Start
-----------

The example has several predefined poses, which can be choosen by ``-n`` flag, as the followng:

* Joint Movement

.. note::
    Pose 0| J0: 0.0°| J1: -15.0°| J2:  0.0°| J3:  90.0°| J4: 0.0 °| J5: 90.0°| J6:  0.0°

    Pose 1| J0: 0.0°| J1:   0.0°| J2: 30.0°| J3: 120.0°| J4: 30.0°| J5: 80.0°| J6: 10.0°

    Pose 2| J0: 0.0°| J1:  25.0°| J2: 50.0°| J3:  80.0°| J4: 60.0°| J5: 60.0°| J6: 30.0°

.. code-block::

 $ ./kord_move_joints_discrete -t 10 -p 7582 -c 192.168.0.99 -n 1

* Linear Movement

.. warning::
    It is not trivial to define representatively good and at the same time safe poses for linear movement, thus:
    
    Pose 0| x: +10%| y: -10%| z: +10%|

    Pose 1| x: -10%| y: +10%| z: -10%|

.. code-block::

 $ ./kord_move_joints_discrete -t 10 -p 7582 -c 192.168.0.99 -n 1


Description
-----------

The provided examples for `joints` and `linear` movements are supposed to put the robot to certain predefined poses 
automatically computed the velocities and accelerations based on the required time.

The full example can be found in ``examples/kord_move_joints_discrete.cpp`` or ``examples/kord_move_linear_discrete.cpp``.
