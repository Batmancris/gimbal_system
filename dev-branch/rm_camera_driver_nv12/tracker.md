# RM Armor Tracker
Based on the chenjun open-source rm_vision framework, 
this project relies on RDK_X5 and Daheng MER-139 industrial camera suite to establish a set of vision tracker solutions

## Changelog
2025-1-8：Upload Basic functions
.
└── src
    └── rm_armor_tracker
        ├── rm_bringup
        ├── rm_camera_driver
        └── rm_utils

## Installation Instructions
### Get source code
`git clone https://github.com/tianbot/rm_armor_tracker.git`
### External library
`sudo apt-get update`
Install Ceres
`sudo apt-get install libceres-dev`
Install camera_info_manager
`sudo apt-get inatsll ros2-humble-camera_info_manager`
Install Transport
`sudo apt-get inatsll ros2-humble-image_transport`
### Cmake
add External library in CMakeList.txt
`find_package(ament_cmake_auto REQUIRED)`
`find_package(camera_info_manager REQUIRED)`
`find_package(image_transport REQUIRED)`
## Usage Instructions
Build project
`cd ~/rm_armor_tracke`
Compile the feature package
`colcon build`
Add environment variables
`source install/setup.bash`
Start camera node

## Start camera node    
Open DaHeng camera node
`ros2 run rm_camera_driver dahengcamera_node`
visualization
`rviz`


## Topics
Image_raw
## License
The tracker_node is prietary. Packages like rm_bringup、rm_camera_driver are under MIT license.
Galaxy SDK is under commercial license.