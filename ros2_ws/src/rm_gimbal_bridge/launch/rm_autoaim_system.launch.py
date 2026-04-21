import os

from ament_index_python.packages import get_package_prefix, get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch_ros.actions import Node


def generate_launch_description():
    bridge_share = get_package_share_directory("rm_gimbal_bridge")
    hik_share = get_package_share_directory("hik_camera")
    vehicle_share = get_package_share_directory("rm_vehicle_detection")
    vehicle_prefix = get_package_prefix("rm_vehicle_detection")
    default_vehicle_model_path = os.path.join(
        vehicle_prefix, "lib", "rm_vehicle_detection", "config", "quant.bin"
    )

    serial_port = LaunchConfiguration("serial_port")
    enemy_prefix = LaunchConfiguration("enemy_prefix")
    camera_params = LaunchConfiguration("camera_params")
    launch_camera = LaunchConfiguration("launch_camera")
    detector_type = LaunchConfiguration("detector_type")
    detector_topic = LaunchConfiguration("detector_topic")
    vehicle_model_path = LaunchConfiguration("vehicle_model_path")

    bridge_config = os.path.join(bridge_share, "config", "rm_gimbal_bridge.yaml")
    hik_launch = os.path.join(hik_share, "launch", "hik_camera.launch.py")
    vehicle_config = os.path.join(vehicle_share, "config", "rm_vehicle_detection.yaml")

    serial_port_arg = DeclareLaunchArgument("serial_port", default_value="/dev/ttyS1")
    enemy_prefix_arg = DeclareLaunchArgument("enemy_prefix", default_value="")
    camera_params_arg = DeclareLaunchArgument(
        "camera_params",
        default_value=os.path.join(hik_share, "config", "camera_params.yaml"),
    )
    launch_camera_arg = DeclareLaunchArgument("launch_camera", default_value="true")
    detector_type_arg = DeclareLaunchArgument("detector_type", default_value="vehicle")
    detector_topic_arg = DeclareLaunchArgument(
        "detector_topic", default_value="/vehicle_detection/targets"
    )
    vehicle_model_path_arg = DeclareLaunchArgument(
        "vehicle_model_path", default_value=default_vehicle_model_path
    )

    shared_mem_node = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                get_package_share_directory("hobot_shm"),
                "launch/hobot_shm.launch.py",
            )
        )
    )

    camera_node = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(
                hik_launch,
            )
        ),
        condition=IfCondition(launch_camera),
        launch_arguments={
            "params_file": camera_params,
        }.items(),
    )

    detection_node = Node(
        package="rm_armor_detection",
        executable="rm_armor_detection",
        name="rm_armor_detection",
        output="screen",
        arguments=["--ros-args", "--log-level", "warn"],
        condition=IfCondition(
            PythonExpression(["'", detector_type, "' == 'armor'"])
        ),
    )

    vehicle_detection_node = Node(
        package="rm_vehicle_detection",
        executable="rm_vehicle_detection_node",
        name="rm_vehicle_detection",
        output="screen",
        parameters=[
            vehicle_config,
            {
                "output_topic": detector_topic,
                "model_path": vehicle_model_path,
            },
        ],
        condition=IfCondition(
            PythonExpression(["'", detector_type, "' == 'vehicle'"])
        ),
    )

    bridge_node = Node(
        package="rm_gimbal_bridge",
        executable="rm_gimbal_bridge_node",
        name="rm_gimbal_bridge",
        output="screen",
        parameters=[
            bridge_config,
            {
                "serial_port": serial_port,
                "enemy_prefix": enemy_prefix,
                "input_topic": detector_topic,
            },
        ],
    )

    return LaunchDescription(
        [
            serial_port_arg,
            enemy_prefix_arg,
            camera_params_arg,
            launch_camera_arg,
            detector_type_arg,
            detector_topic_arg,
            vehicle_model_path_arg,
            shared_mem_node,
            camera_node,
            detection_node,
            vehicle_detection_node,
            bridge_node,
        ]
    )
