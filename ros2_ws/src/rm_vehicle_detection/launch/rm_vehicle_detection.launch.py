import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    share_dir = get_package_share_directory("rm_vehicle_detection")
    default_config = os.path.join(share_dir, "config", "rm_vehicle_detection.yaml")
    default_model_path = "/opt/tros/lib/rm_vehicle_detection/config/quant.bin"

    config_arg = DeclareLaunchArgument("config_file", default_value=default_config)
    image_topic_arg = DeclareLaunchArgument("image_topic", default_value="/hbmem_img")
    output_topic_arg = DeclareLaunchArgument(
        "output_topic", default_value="/vehicle_detection/targets"
    )
    model_path_arg = DeclareLaunchArgument("model_path", default_value=default_model_path)

    node = Node(
        package="rm_vehicle_detection",
        executable="rm_vehicle_detection_node",
        name="rm_vehicle_detection",
        output="screen",
        parameters=[
            LaunchConfiguration("config_file"),
            {
                "image_topic": LaunchConfiguration("image_topic"),
                "output_topic": LaunchConfiguration("output_topic"),
                "model_path": LaunchConfiguration("model_path"),
            },
        ],
    )

    return LaunchDescription([config_arg, image_topic_arg, output_topic_arg, model_path_arg, node])
