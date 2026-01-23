#!/usr/bin/env python3
import os
import yaml
from launch import LaunchDescription
from launch.actions import ExecuteProcess, DeclareLaunchArgument, TimerAction
from launch.conditions import UnlessCondition, IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
from moveit_configs_utils import MoveItConfigsBuilder


def load_yaml(package_name, file_path):
    package_path = get_package_share_directory(package_name)
    absolute_file_path = os.path.join(package_path, file_path)
    try:
        with open(absolute_file_path, "r") as file:
            return yaml.safe_load(file)
    except EnvironmentError:
        return None


def generate_launch_description():
    import sys
    using_fake_hardware = any('use_fake_hardware:=true' in arg for arg in sys.argv)

    declared_arguments = [
        DeclareLaunchArgument(
            "use_fake_hardware",
            default_value="false",
            description="Use mock hardware (true) or real Kassow robot (false)",
        ),
        DeclareLaunchArgument(
            "robot_ip",
            default_value="192.168.1.100",
            description="IP address of the Kassow robot",
        ),
        DeclareLaunchArgument(
            "gripper_tcp_port",
            default_value="5000",
            description="TCP server port for OnRobot gripper",
        ),
        DeclareLaunchArgument(
            "gripper_force",
            default_value="20.0",
            description="Default gripper force in Newtons",
        ),
        DeclareLaunchArgument(
            "joystick_device",
            default_value="",
            description="Joystick device path (e.g., /dev/input/event5). Empty = auto-detect",
        ),
    ]

    use_fake_hardware = LaunchConfiguration("use_fake_hardware")
    robot_ip = LaunchConfiguration("robot_ip")
    gripper_tcp_port = LaunchConfiguration("gripper_tcp_port")
    gripper_force = LaunchConfiguration("gripper_force")
    joystick_device = LaunchConfiguration("joystick_device")

    kr810_rg2_description_dir = get_package_share_directory('kr810_rg2_description')
    joystick_control_share = get_package_share_directory('joystick_control')

    controllers_file = os.path.join(
        kr810_rg2_description_dir,
        "config",
        "moveit_controllers_fake.yaml" if using_fake_hardware else "moveit_controllers.yaml"
    )

    moveit_config = (
        MoveItConfigsBuilder("kr810_rg2", package_name="kr810_rg2_description")
        .robot_description(
            file_path=os.path.join(kr810_rg2_description_dir, "urdf", "kr810_rg2.urdf.xacro"),
            mappings={
                "use_fake_hardware": use_fake_hardware,
                "robot_ip": robot_ip,
            }
        )
        .robot_description_semantic(file_path=os.path.join(kr810_rg2_description_dir, "config", "kr810_rg2.srdf"))
        .robot_description_kinematics(file_path=os.path.join(kr810_rg2_description_dir, "config", "kinematics.yaml"))
        .joint_limits(file_path=os.path.join(kr810_rg2_description_dir, "config", "joint_limits.yaml"))
        .trajectory_execution(file_path=controllers_file)
        .planning_pipelines(pipelines=["ompl"], default_planning_pipeline="ompl")
        .pilz_cartesian_limits(file_path=os.path.join(kr810_rg2_description_dir, "config", "pilz_cartesian_limits.yaml"))
        .to_moveit_configs()
    )

    move_group_capabilities = {
        "capabilities": "move_group/ExecuteTaskSolutionCapability"
    }

    run_move_group_node = Node(
        package="moveit_ros_move_group",
        executable="move_group",
        output="screen",
        parameters=[
            moveit_config.to_dict(),
            move_group_capabilities,
        ],
    )

    static_tf = Node(
        package="tf2_ros",
        executable="static_transform_publisher",
        name="static_transform_publisher",
        output="log",
        arguments=["--frame-id", "world", "--child-frame-id", "base"],
    )

    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        name="robot_state_publisher",
        output="both",
        parameters=[moveit_config.robot_description],
    )

    ros2_controllers_path = os.path.join(
        kr810_rg2_description_dir, "config", "ros2_controllers.yaml"
    )
    ros2_control_node = Node(
        package="controller_manager",
        executable="ros2_control_node",
        parameters=[ros2_controllers_path],
        remappings=[
            ("/controller_manager/robot_description", "/robot_description"),
        ],
        output="both",
    )

    load_controllers = []
    for controller in ["joint_state_broadcaster", "kr810_arm_controller"]:
        load_controllers += [
            ExecuteProcess(
                cmd=["ros2 run controller_manager spawner {}".format(controller)],
                shell=True,
                output="screen",
            )
        ]

    load_controllers += [
        ExecuteProcess(
            cmd=["ros2 run controller_manager spawner rg2_gripper_controller --controller-manager-timeout 10"],
            shell=True,
            output="screen",
            condition=IfCondition(use_fake_hardware)
        )
    ]

    tcp_gripper_controller = Node(
        package='mtc_demo',
        executable='onrobot_tcp_controller.py',
        name='onrobot_tcp_controller',
        output='screen',
        parameters=[{
            'tcp_host': robot_ip,
            'tcp_port': gripper_tcp_port,
            'default_force': gripper_force,
            'max_width': 110.0,
            'min_width': 0.0,
            'socket_timeout': 5.0,
        }],
        condition=UnlessCondition(use_fake_hardware)
    )

    mtc_node = Node(
        package="mtc_demo",
        executable="mtc_node",
        output="screen",
        parameters=[moveit_config.to_dict()],
    )

    delayed_mtc_node = TimerAction(
        period=4.0,
        actions=[mtc_node]
    )

    joystick_params_file = os.path.join(
        joystick_control_share, 'config', 'joystick_teleop_params.yaml'
    )
    servo_yaml_path = os.path.join(
        joystick_control_share, 'config', 'moveit_servo.yaml'
    )

    with open(servo_yaml_path, 'r') as f:
        servo_yaml = yaml.safe_load(f)
    servo_params = {"moveit_servo": servo_yaml}

    srdf_path = os.path.join(kr810_rg2_description_dir, 'config', 'kr810_rg2.srdf')
    with open(srdf_path, 'r') as f:
        robot_description_semantic_config = f.read()

    kinematics_yaml = load_yaml('kr810_rg2_description', 'config/kinematics.yaml')

    servo_node_params = [
        servo_params,
        {'robot_description_semantic': robot_description_semantic_config},
        moveit_config.to_dict(),
    ]
    if kinematics_yaml:
        servo_node_params.append(kinematics_yaml)

    joystick_teleop_node = Node(
        package='joystick_control',
        executable='joystick_teleop',
        name='joystick_teleop_node',
        output='screen',
        parameters=[
            joystick_params_file,
            {'joystick_device': joystick_device}
        ],
    )

    servo_node = Node(
        package='moveit_servo',
        executable='servo_node_main',
        name='servo_node',
        output='screen',
        parameters=servo_node_params,
    )

    servo_starter = Node(
        package='joystick_control',
        executable='start_servo',
        name='servo_starter',
        output='screen',
    )

    delayed_joystick_nodes = TimerAction(
        period=5.0,
        actions=[joystick_teleop_node, servo_node, servo_starter]
    )

    return LaunchDescription(
        declared_arguments
        + [
            run_move_group_node,
            static_tf,
            robot_state_publisher,
            ros2_control_node,
        ]
        + load_controllers
        + [
            tcp_gripper_controller,
            delayed_mtc_node,
            delayed_joystick_nodes,
        ]
    )


if __name__ == '__main__':
    generate_launch_description()