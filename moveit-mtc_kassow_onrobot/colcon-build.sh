#!/bin/bash

# Source ROS environment first (needed for rosdep to work correctly)
source /opt/ros/humble/setup.bash

# Check and install libnet1-dev for OnRobot driver Modbus support
if ! dpkg -l | grep -q "libnet1-dev"; then
    echo "libnet1-dev not found. Please install it manually:"
    echo "  sudo apt-get install -y libnet1-dev"
    exit 1
else
    echo "libnet1-dev is already installed."
fi

# Install dependencies with rosdep (skip ament_python - already in ROS 2)
rosdep install -i --from-path src --rosdistro humble -y --skip-keys "ament_python"
if [ $? -eq 0 ]
then
    echo " "
else
    exit 1
fi

#rm -rf install/
# mkdir -p install/mtc_tutorial
#rm -rf build/
#rm -rf log/

# Build with colcon
colcon build --mixin release --merge-install
#colcon build --packages-select moveit_task_constructor_visualization
