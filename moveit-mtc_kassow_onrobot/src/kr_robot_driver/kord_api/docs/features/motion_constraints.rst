.. _motion_constraints:

==================
Motion Constraints
==================

Motion constraints in KORD define the operational limits of the robot's movements to ensure safe and efficient performance.

.. important::
   Motion constraints serve as a **preliminary check** to ensure the robot's movements are within the operational limits.
   This is not a substitute for the robot's internal safety features, which are always active.

.. note::

   Motion constraints are enforced by KORD. Movement commands that exceed these constraints are blocked and not transmitted to the robot controller.

If a violation occurs, a system event is generated with the following details:

- **Group**: :cpp:enumerator:`kr2::kord::protocol::EEventGroup::eKordEvent`.
- **ID**: :cpp:enumerator:`kr2::kord::protocol::EKordEventConditionID::INFEASIBLE_MOVE_COMMAND`.

Overview
========

Motion constraints are categorized into two main types:

1. **Joint Space Constraints:** Control the speed and acceleration of individual joints.
2. **Work Space Constraints:** Control the speed, acceleration, and orientation change speed of the end-effector.

Supported tracking types:

- :cpp:enumerator:`kr2::kord::TrackingType::TT_TIME`
- :cpp:enumerator:`kr2::kord::TrackingType::TT_JS_TARGET_SPEED`
- :cpp:enumerator:`kr2::kord::TrackingType::TT_WS_TARGET_SPEED`

Blending type does not affect the constraint checks.

Methodology
-----------

The constraint checks use simplified motion models to ensure safety margins.
Actual robot motion might follow more complex profiles, but the verification
uses conservative linear estimations that guarantee never underestimating
movement demands.

- **Tracking Time Determination:**

  - Directly uses user-provided duration when tracking type is :cpp:enumerator:`kr2::kord::TrackingType::TT_TIME`.
  - Estimates duration automatically when using :cpp:enumerator:`kr2::kord::TrackingType::TT_JS_TARGET_SPEED` or :cpp:enumerator:`kr2::kord::TrackingType::TT_WS_TARGET_SPEED`.

    - *Joint space:* Based on the maximum angle difference:

      .. math::

         t = \frac{\Delta \theta_{\text{max}}}{v_{\text{target}}},

      where :math:`\Delta \theta_{\text{max}}` is the maximum angle difference between the current and target joint positions.

    - *Workspace:* Based on the maximum between the linear and rotational speed:

      .. math::

         t = \max\left(\frac{d}{v_{\text{target}}}, \frac{\Delta\phi}{\omega_{\text{max}}}\right),

      where:

      - :math:`d` = Euclidean distance between positions.
      - :math:`\Delta\phi` = Angular difference between orientations.
      - :math:`v_{\text{target}}` = Configured ``max_ws_speed`` (m/s).
      - :math:`\omega_{\text{max}}` = Maximum orientation change speed (rad/s) from ``max_ws_orientation_speed`` parameter.

- **Joint Space Checks:**

  - **Speed estimation:** :math:`v = \frac{\Delta\theta}{t}`  
    (where :math:`\Delta\theta` accounts for full rotations).
  - **Acceleration estimation:** :math:`a = \frac{v_{\text{prev}} - v_{\text{current}}}{t}`.
  - *Note:* Uses a simple linear estimation assuming instantaneous changes.

- **Workspace Checks:**

  - **Linear speed:** :math:`v_{\text{linear}} = \frac{d}{t}`, where :math:`d` is the Euclidean norm (distance) between positions.
  - **Rotation speed:** :math:`\omega = \frac{\Delta\phi}{t}`, where :math:`\Delta\phi` represents the angular difference between orientations, calculated using quaternion spherical interpolation.

- **Validation Approach:**

  - Conservative estimates using worst-case scenarios.
  - Parallel checking of all constraints.
  - First violation blocks movement and triggers logging.
  - *Heuristic aspect:* Assumes constant acceleration profile for simplicity.

Configuration
=============

Below are the key parameters used to configure motion constraints:

Joint Space Constraints
-----------------------

+-----------------------------+--------------------------------------------+
| **Parameter**               | **Description**                            |
+=============================+============================================+
| ``enabled``                 | Enable or disable motion constraints.      |
+-----------------------------+--------------------------------------------+
| ``max_joint1_speed``        | Maximum speed for joint 1 (deg/s).         |
+-----------------------------+--------------------------------------------+
| ``max_joint2_speed``        | Maximum speed for joint 2 (deg/s).         |
+-----------------------------+--------------------------------------------+
| ``max_joint3_speed``        | Maximum speed for joint 3 (deg/s).         |
+-----------------------------+--------------------------------------------+
| ``max_joint4_speed``        | Maximum speed for joint 4 (deg/s).         |
+-----------------------------+--------------------------------------------+
| ``max_joint5_speed``        | Maximum speed for joint 5 (deg/s).         |
+-----------------------------+--------------------------------------------+
| ``max_joint6_speed``        | Maximum speed for joint 6 (deg/s).         |
+-----------------------------+--------------------------------------------+
| ``max_joint7_speed``        | Maximum speed for joint 7 (deg/s).         |
+-----------------------------+--------------------------------------------+
| ``max_joint1_acceleration`` | Maximum acceleration for joint 1 (deg/s²). |
+-----------------------------+--------------------------------------------+
| ``max_joint2_acceleration`` | Maximum acceleration for joint 2 (deg/s²). |
+-----------------------------+--------------------------------------------+
| ``max_joint3_acceleration`` | Maximum acceleration for joint 3 (deg/s²). |
+-----------------------------+--------------------------------------------+
| ``max_joint4_acceleration`` | Maximum acceleration for joint 4 (deg/s²). |
+-----------------------------+--------------------------------------------+
| ``max_joint5_acceleration`` | Maximum acceleration for joint 5 (deg/s²). |
+-----------------------------+--------------------------------------------+
| ``max_joint6_acceleration`` | Maximum acceleration for joint 6 (deg/s²). |
+-----------------------------+--------------------------------------------+
| ``max_joint7_acceleration`` | Maximum acceleration for joint 7 (deg/s²). |
+-----------------------------+--------------------------------------------+

Work Space Constraints
----------------------

+-------------------------------------+---------------------------------------------+
| **Parameter**                       | **Description**                             |
+=====================================+=============================================+
| ``max_ws_speed``                    | Maximum speed in the workspace (m/s).       |
+-------------------------------------+---------------------------------------------+
| ``max_ws_acceleration``             | Maximum acceleration in the workspace       |
|                                     | (m/s²).                                     |
+-------------------------------------+---------------------------------------------+
| ``max_ws_orientation_speed``        | Maximum orientation change speed (rad/s).   |
+-------------------------------------+---------------------------------------------+

Configuration Example
=====================

The following example shows how to configure motion constraints in the ``KORD.ini`` file:

.. code-block:: ini
   :caption: Example Motion Constraints Configuration

   [MOTION_CONSTRAINTS]
   enabled = yes

   ; Joint space - deg/s
   max_joint1_speed = 225.0
   max_joint2_speed = 225.0
   max_joint3_speed = 225.0
   max_joint4_speed = 225.0
   max_joint5_speed = 225.0
   max_joint6_speed = 225.0
   max_joint7_speed = 225.0

   ; Joint space - deg/s²
   max_joint1_acceleration = 10000.0
   max_joint2_acceleration = 10000.0
   max_joint3_acceleration = 10000.0
   max_joint4_acceleration = 10000.0
   max_joint5_acceleration = 10000.0
   max_joint6_acceleration = 10000.0
   max_joint7_acceleration = 10000.0

   ; Workspace - m/s
   max_ws_speed = 2.0

   ; Workspace - m/s²
   max_ws_acceleration = 25.0

   ; Workspace - orientation speed - rad/s
   max_ws_orientation_speed = 1.0

Best Practices
==============

To make the most of motion constraints:

- **Set Realistic Limits:** Configure speed and acceleration values that are within the robot's physical capabilities and operational safety margins.
- **Test Incrementally:** Start with conservative values and gradually increase them while testing to ensure the robot performs as expected.

Troubleshooting
===============

If movement commands are not executed as expected, consider the following:

1. **Verify Configuration:** Check the ``MOTION_CONSTRAINTS`` section of the ``KORD.ini`` file to ensure values are set correctly.

2. **Ensure Compatibility:** Ensure the robot's hardware and software support the configured constraints.

3. **Monitor Logs:** Review the log files to identify issues related to motion constraints.
