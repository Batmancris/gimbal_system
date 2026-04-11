from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
import os


def generate_launch_description():
    config = os.path.join(
        get_package_share_directory("rm_gimbal_bridge"),
        "config",
        "rm_gimbal_bridge.yaml",
    )

    return LaunchDescription(
        [
            Node(
                package="rm_gimbal_bridge",
                executable="rm_gimbal_bridge_node",
                name="rm_gimbal_bridge",
                output="screen",
                parameters=[config],
            )
        ]
    )
