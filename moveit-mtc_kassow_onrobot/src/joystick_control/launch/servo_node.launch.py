#!/usr/bin/env python3

"""
Launch file for MoveIt Servo node

This properly configures and launches servo_node_main with all required parameters.
"""

import os
import yaml
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import Command
from launch_ros.parameter_descriptions import ParameterValue
from ament_index_python.packages import get_package_share_directory


def load_yaml(package_name, file_path):
    package_path = get_package_share_directory(package_name)
    absolute_file_path = os.path.join(package_path, file_path)

    try:
        with open(absolute_file_path, "r") as file:
            return yaml.safe_load(file)
    except EnvironmentError:
        return None


def generate_launch_description():
    # Load servo configuration
    joystick_control_share = get_package_share_directory('joystick_control')
    servo_yaml_path = os.path.join(joystick_control_share, 'config', 'moveit_servo.yaml')

    with open(servo_yaml_path, 'r') as f:
        servo_yaml = yaml.safe_load(f)

    servo_params = {"moveit_servo": servo_yaml}

    # Load robot description files
    kr810_description_share = get_package_share_directory('kr810_rg2_description')

    # Process URDF with xacro
    urdf_path = os.path.join(kr810_description_share, 'urdf', 'kr810_rg2.urdf.xacro')
    robot_description_content = Command(['xacro ', urdf_path, ' use_fake_hardware:=true'])
    robot_description_config = ParameterValue(robot_description_content, value_type=str)

    # Load SRDF
    srdf_path = os.path.join(kr810_description_share, 'config', 'kr810_rg2.srdf')
    robot_description_semantic_config = open(srdf_path).read()

    # Load kinematics
    kinematics_yaml = load_yaml('kr810_rg2_description', 'config/kinematics.yaml')

    # Combine all parameters
    servo_node_params = [
        servo_params,
        {
            'robot_description': robot_description_config,
            'robot_description_semantic': robot_description_semantic_config,
        }
    ]

    if kinematics_yaml:
        servo_node_params.append(kinematics_yaml)

    # Launch servo node
    servo_node = Node(
        package="moveit_servo",
        executable="servo_node_main",
        name="servo_node",
        output="screen",
        parameters=servo_node_params,
    )

    return LaunchDescription([servo_node])
