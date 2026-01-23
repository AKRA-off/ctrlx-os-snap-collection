# MoveIt MTC for Kassow Robots with OnRobot gripper

## Introduction

The project demonstrates ROS2 control of Kassow Cobot over KORD API with Joystick integration along with OnRobot gripper control functionality with a custom CBun.

## Function Description

Use Your Xbox Series Controller to switch between teach and automatic mode, teach movement points and execute Pick and Place sequence.
Requires ros2-base snap for running.

## Implementation Description 

The module __data__ contains .yaml file with teach positions
__snap__ including snapcraft.yaml
__src__ with all required dependencies for MTC, MoveIt task constructor, Joystick, Kassow Robot and OnRobot gripper
__wrapper__ with all the required scripts to run
__setup.py__



## Build

Designed for CtrlX Core X5 and X7.
Download the source code and build the snap executing __build-snap.sh__ script:

```shell
snapcraft --destructive-mode
```



___

## License

SPDX-FileCopyrightText: Bosch Rexroth AG
SPDX-License-Identifier: MIT
