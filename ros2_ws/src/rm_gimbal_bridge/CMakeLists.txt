cmake_minimum_required(VERSION 3.8)
project(rm_gimbal_bridge)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(ai_msgs QUIET)

set(rm_gimbal_bridge_targets usb_cdc_ping_test usb_cdc_pitch_test)

if(ai_msgs_FOUND)
  add_executable(${PROJECT_NAME}_node
    src/serial_bridge_node.cpp
  )

  add_executable(rm_gimbal_feedback_node
    src/target_feedback_node.cpp
  )

  target_include_directories(${PROJECT_NAME}_node PUBLIC include)
  target_include_directories(rm_gimbal_feedback_node PUBLIC include)

  ament_target_dependencies(${PROJECT_NAME}_node
    rclcpp
    ai_msgs
  )

  ament_target_dependencies(rm_gimbal_feedback_node
    rclcpp
    ai_msgs
  )

  list(APPEND rm_gimbal_bridge_targets ${PROJECT_NAME}_node rm_gimbal_feedback_node)
else()
  message(WARNING "ai_msgs not found, skipping ${PROJECT_NAME}_node and building usb_cdc_ping_test only")
endif()

add_executable(usb_cdc_ping_test
  src/usb_cdc_ping_test.cpp
)

add_executable(usb_cdc_pitch_test
  src/usb_cdc_pitch_test.cpp
)

install(TARGETS
  ${rm_gimbal_bridge_targets}
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  config
  launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
