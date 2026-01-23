#!/bin/bash

# Install MoveIt Servo for ROS2 Humble

echo "Installing MoveIt Servo..."

# Update package list
sudo apt update

# Install MoveIt Servo
sudo apt install -y ros-humble-moveit-servo
