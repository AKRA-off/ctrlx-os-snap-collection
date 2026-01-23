#!/bin/bash
# Launch RViz visualization for KR810 + RG2
# Automatically includes gripper visualization with static mesh swapping

source /opt/ros/humble/setup.bash
source ./install/local_setup.bash

exec ros2 launch mtc_demo visualization_kr810.launch.py


