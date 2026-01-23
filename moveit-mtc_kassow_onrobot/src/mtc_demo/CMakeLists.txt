cmake_minimum_required(VERSION 3.8)
project(mtc_demo)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(moveit_task_constructor_core REQUIRED)
find_package(rclcpp REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(ament_index_cpp REQUIRED)

add_executable(mtc_node src/mtc_node.cpp)
target_include_directories(mtc_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(mtc_node PUBLIC c_std_99 cxx_std_17)  # Require C99 and C++17
ament_target_dependencies(mtc_node "moveit_task_constructor_core" "rclcpp" "ament_index_cpp")
target_link_libraries(mtc_node yaml-cpp)

install(TARGETS mtc_node
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})

# Install data directory for runtime writable files (taught positions, etc.)
install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/../../data
  DESTINATION share/${PROJECT_NAME}
  USE_SOURCE_PERMISSIONS)

install(PROGRAMS
  scripts/onrobot_tcp_controller.py
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  # set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  # set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

 ament_package()
