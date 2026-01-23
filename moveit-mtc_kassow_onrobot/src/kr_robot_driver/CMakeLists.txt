cmake_minimum_required(VERSION 3.16)
project(kr_robot_driver LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
  add_compile_options(-Wall -Wextra)
endif()

# set the same behavior for windows as it is on linux
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# find dependencies
set(HW_IF_INCLUDE_DEPENDS
  pluginlib
  rcpputils
  hardware_interface
)
set(REF_GEN_INCLUDE_DEPENDS
  kdl_parser
  rclcpp
  trajectory_msgs
)
set(CONTROLLER_INCLUDE_DEPENDS
  pluginlib
  rcpputils
  controller_interface
  realtime_tools
  trajectory_msgs
)

# Specify the required version of ros2_control
# Note: Lowered from 4.0.0 to 2.0.0 for ROS 2 Humble compatibility
find_package(controller_manager 2.0.0)
# Handle the case where the required version is not found
if(NOT controller_manager_FOUND)
  message(FATAL_ERROR "ros2_control version 2.0.0 or higher is required.")
endif()

# find dependencies
find_package(backward_ros REQUIRED)
find_package(ament_cmake REQUIRED)
foreach(Dependency IN ITEMS ${HW_IF_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()
foreach(Dependency IN ITEMS ${REF_GEN_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()
foreach(Dependency IN ITEMS ${CONTROLLER_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

# Add KORD API subdirectory for real robot communication
# KORD protocol and ASIO dependencies are now available
add_subdirectory(kord_api)

## COMPILE
add_executable(send_trajectory reference_generator/send_trajectory.cpp)

ament_target_dependencies(
  send_trajectory PUBLIC
  ${REF_GEN_INCLUDE_DEPENDS}
)

add_library(
  kr_robot_driver
  SHARED
  hardware/kr810_hardware.cpp
  controller/kr810_controller.cpp
)

target_compile_features(kr_robot_driver PUBLIC cxx_std_17)
target_compile_definitions(kr_robot_driver PRIVATE ENABLE_REAL_ROBOT)
target_include_directories(kr_robot_driver PUBLIC
$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/hardware/include>
$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/controller/include>
$<INSTALL_INTERFACE:include/kr_robot_driver>
)
# Note: kord-api target provides its own include directories (kord_api/include, external/kord, and fetched kord-protocol)
ament_target_dependencies(
  kr_robot_driver PUBLIC
  ${HW_IF_INCLUDE_DEPENDS}
  ${CONTROLLER_INCLUDE_DEPENDS}
)

# Link KORD API library for real robot communication
# Use PRIVATE to avoid export issues
target_link_libraries(kr_robot_driver PRIVATE kord-api)

# Export hardware plugins
pluginlib_export_plugin_description_file(hardware_interface kr_robot_driver.xml)
# Export controller plugins
pluginlib_export_plugin_description_file(controller_interface kr_robot_driver.xml)

# INSTALL
install(
  DIRECTORY hardware/include/
  DESTINATION include/kr_robot_driver
)
install(
  DIRECTORY description/launch description/ros2_control description/urdf
  DESTINATION share/kr_robot_driver
)
install(
  DIRECTORY bringup/launch bringup/config
  DESTINATION share/kr_robot_driver
)
install(
    TARGETS send_trajectory
    RUNTIME DESTINATION lib/kr_robot_driver
)

install(TARGETS kr_robot_driver kord-api
  EXPORT export_kr_robot_driver
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

if(BUILD_TESTING)
  find_package(ament_cmake_pytest REQUIRED)

  ament_add_pytest_test(example_7_urdf_xacro test/test_urdf_xacro.py)
  ament_add_pytest_test(view_example_7_launch test/test_view_robot_launch.py)
  ament_add_pytest_test(run_example_7_launch test/test_r6bot_controller_launch.py)
endif()

## EXPORTS
ament_export_targets(export_kr_robot_driver HAS_LIBRARY_TARGET)
ament_export_dependencies(${HW_IF_INCLUDE_DEPENDS} ${REF_GEN_INCLUDE_DEPENDS} ${CONTROLLER_INCLUDE_DEPENDS})
ament_package()
