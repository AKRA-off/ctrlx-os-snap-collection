#!/usr/bin/env python3

"""
Launch file for Xbox joystick teleoperation

This launch file starts:
1. Joystick teleop node - reads Xbox controller and publishes twist commands
2. MoveIt Servo - converts twist commands to joint commands for real-time control
"""

import os
import yaml
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, Command
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
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
    # Declare launch arguments
    enable_servo_arg = DeclareLaunchArgument(
        'enable_servo',
        default_value='true',
        description='Enable MoveIt Servo for real-time control'
    )

    enable_collision_checking_arg = DeclareLaunchArgument(
        'enable_collision_checking',
        default_value='false',
        description='Enable collision checking in MoveIt Servo'
    )

    # Get package paths
    joystick_control_share = get_package_share_directory('joystick_control')
    kr810_description_share = get_package_share_directory('kr810_rg2_description')

    # Configuration files
    joystick_params_file = os.path.join(
        joystick_control_share,
        'config',
        'joystick_teleop_params.yaml'
    )

    servo_yaml_path = os.path.join(
        joystick_control_share,
        'config',
        'moveit_servo.yaml'
    )

    # Load servo configuration
    with open(servo_yaml_path, 'r') as f:
        servo_yaml = yaml.safe_load(f)
    servo_params = {"moveit_servo": servo_yaml}

    # Process URDF with xacro
    urdf_path = os.path.join(kr810_description_share, 'urdf', 'kr810_rg2.urdf.xacro')
    robot_description_content = Command(['xacro ', urdf_path, ' use_fake_hardware:=true'])
    robot_description_config = ParameterValue(robot_description_content, value_type=str)

    # Load SRDF
    srdf_path = os.path.join(kr810_description_share, 'config', 'kr810_rg2.srdf')
    with open(srdf_path, 'r') as f:
        robot_description_semantic_config = f.read()

    # Load kinematics
    kinematics_yaml = load_yaml('kr810_rg2_description', 'config/kinematics.yaml')

    # Combine all servo parameters
    servo_node_params = [
        servo_params,
        {
            'robot_description': robot_description_config,
            'robot_description_semantic': robot_description_semantic_config,
        }
    ]

    if kinematics_yaml:
        servo_node_params.append(kinematics_yaml)

    # Joystick teleop node
    joystick_teleop_node = Node(
        package='joystick_control',
        executable='joystick_teleop',
        name='joystick_teleop_node',
        output='screen',
        parameters=[joystick_params_file],
    )

    # MoveIt Servo node with proper parameters
    servo_node = Node(
        package='moveit_servo',
        executable='servo_node_main',
        name='servo_node',
        output='screen',
        parameters=servo_node_params,
        condition=IfCondition(LaunchConfiguration('enable_servo')),
    )

    # Servo starter node - automatically starts servo after it's ready
    servo_starter = Node(
        package='joystick_control',
        executable='start_servo',
        name='servo_starter',
        output='screen',
        condition=IfCondition(LaunchConfiguration('enable_servo')),
    )

    return LaunchDescription([
        enable_servo_arg,
        enable_collision_checking_arg,
        joystick_teleop_node,
        servo_node,
        servo_starter,
    ])
