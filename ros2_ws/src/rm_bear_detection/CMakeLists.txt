cmake_minimum_required(VERSION 3.8)
project(rm_bear_detection)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(ai_msgs REQUIRED)
find_package(hbm_img_msgs REQUIRED)
find_package(dnn_node REQUIRED)
find_package(hobot_cv REQUIRED)
find_package(OpenCV REQUIRED)

set(PREFIX_PATH x3)
set(SYS_ROOT ${CMAKE_SYSROOT})

if(PLATFORM_X3)
  add_definitions(-DPLATFORM_X3)
  set(PREFIX_PATH x3)
elseif(PLATFORM_Rdkultra)
  add_definitions(-DPLATFORM_Rdkultra)
  set(PREFIX_PATH rdkultra)
elseif(PLATFORM_X86)
  add_definitions(-DPLATFORM_X86)
  set(PREFIX_PATH x86)
  set(SYS_ROOT ${THIRD_PARTY})
else()
  if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    add_definitions(-DPLATFORM_X3)
    set(PREFIX_PATH x3)
  elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    add_definitions(-DPLATFORM_X86)
    set(PREFIX_PATH x86)
    set(SYS_ROOT ${THIRD_PARTY})
  endif()
endif()

set(HOBOTCV_LIB_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/lib)
if(hobot_cv_LIBRARIES)
  string(FIND ${hobot_cv_LIBRARIES} "/" index REVERSE)
  string(SUBSTRING ${hobot_cv_LIBRARIES} 0 ${index} HOBOTCV_LIB_INSTALL_PATH)
endif()

link_directories(
  ${SYS_ROOT}/usr/lib/
  ${SYS_ROOT}/usr/hobot/lib/
  ${SYS_ROOT}/usr/lib/hbbpu/
  ${SYS_ROOT}/usr/lib/hbmedia/
  ${HOBOTCV_LIB_INSTALL_PATH}
)

include_directories(
  include
  ${PROJECT_SOURCE_DIR}
  ${SYS_ROOT}/usr/include
  ${SYS_ROOT}/usr/hobot/include
)

add_executable(rm_bear_detection_node
  src/bear_detection_node.cpp
  src/parser.cpp
)

ament_target_dependencies(rm_bear_detection_node
  rclcpp
  std_msgs
  ai_msgs
  ament_index_cpp
  hbm_img_msgs
  dnn_node
  hobot_cv
)

target_link_libraries(rm_bear_detection_node
  ${OpenCV_LIBS}
)

install(
  TARGETS rm_bear_detection_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(
  FILES config/bear_yolov8n_x5_640_nv12.bin
  DESTINATION lib/${PROJECT_NAME}/config
)

install(
  DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
