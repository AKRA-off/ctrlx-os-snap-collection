#!/bin/bash
source $SNAP/usr/bin/setup-paths.sh
# Launch complete joystick + MTC system with mode-based control
# Usage:
#   Simulation: run-joystick-mtc-complete.sh use_fake_hardware:=true
#   Real robot: run-joystick-mtc-complete.sh use_fake_hardware:=false robot_ip:=192.168.1.100
#   With explicit joystick device: add joystick_device:=/dev/input/event5
exec $ROS_BASE/opt/ros/humble/bin/ros2 launch mtc_demo joystick_mtc_complete.launch.py "$@"
