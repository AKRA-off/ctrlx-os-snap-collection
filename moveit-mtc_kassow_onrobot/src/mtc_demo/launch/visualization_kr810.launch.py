import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
from moveit_configs_utils import MoveItConfigsBuilder


def generate_launch_description():
    # Declare launch arguments
    declared_arguments = [
        DeclareLaunchArgument(
            "use_fake_hardware",
            default_value="true",
            description="Use mock hardware (true) or real Kassow robot (false)",
        ),
        DeclareLaunchArgument(
            "robot_ip",
            default_value="192.168.1.100",
            description="IP address of the Kassow robot KORD server",
        ),
    ]

    # Get launch arguments
    use_fake_hardware = LaunchConfiguration("use_fake_hardware")
    robot_ip = LaunchConfiguration("robot_ip")

    # Get package directory
    kr810_rg2_description_dir = get_package_share_directory('kr810_rg2_description')
    mtc_demo_dir = get_package_share_directory('mtc_demo')

    # Build MoveIt configuration for KR810 + RG2
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
        .planning_pipelines(pipelines=["ompl"])
        .to_moveit_configs()
    )

    # RViz node
    rviz_config_file = os.path.join(mtc_demo_dir, "launch", "mtc_kr810.rviz")

    # Use default RViz config if KR810-specific one doesn't exist
    if not os.path.exists(rviz_config_file):
        rviz_config_file = os.path.join(mtc_demo_dir, "launch", "mtc.rviz")

    rviz_node = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="log",
        arguments=["-d", rviz_config_file],
        parameters=[
            moveit_config.to_dict(),
        ],
    )

    return LaunchDescription(declared_arguments + [rviz_node])
