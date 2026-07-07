// ROS2 functionality
#include <rclcpp/rclcpp.hpp>

// Service for executing MTC task
#include <std_srvs/srv/trigger.hpp>

// YAML file reading
#include <fstream>
#include <yaml-cpp/yaml.h>

// Package path resolution
#include <ament_index_cpp/get_package_share_directory.hpp>

// Functionality to interface with the robot model and collision objects
#include <moveit/planning_scene/planning_scene.h>
#include <moveit/planning_scene_interface/planning_scene_interface.h>

// Components of the MoveIt Task Constructor that are used in the example
#include <moveit/task_constructor/task.h>
#include <moveit/task_constructor/solvers.h>
#include <moveit/task_constructor/stages.h>

// Not used in the basic example, later used for pose generation
#if __has_include(<tf2_geometry_msgs/tf2_geometry_msgs.hpp>)
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#else
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#endif
#if __has_include(<tf2_eigen/tf2_eigen.hpp>)
#include <tf2_eigen/tf2_eigen.hpp>
#else
#include <tf2_eigen/tf2_eigen.h>
#endif

// Get a logger for the new node
static const rclcpp::Logger LOGGER = rclcpp::get_logger("mtc_tutorial");

// Create a namespace alias 
namespace mtc = moveit::task_constructor;

// Main class
class MTCTaskNode
{
public:
  MTCTaskNode(const rclcpp::NodeOptions& options);

  rclcpp::node_interfaces::NodeBaseInterface::SharedPtr getNodeBaseInterface();

  void doTask();

  void setupPlanningScene();

private:
  // Compose an MTC task from a series of stages.
  mtc::Task createTask();

  // Service callback for executing MTC task
  void executeTaskCallback(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response);

  // Load taught positions from YAML file
  bool loadTaughtPositions(std::map<std::string, double>& pick_joints,
                           std::map<std::string, double>& place_joints,
                           std::map<std::string, double>& home_joints,
                           double& gripper_close_width);

  mtc::Task task_;
  rclcpp::Node::SharedPtr node_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr execute_task_service_;
};

// Class Constructor - initialices the node
MTCTaskNode::MTCTaskNode(const rclcpp::NodeOptions& options)
  : node_{ std::make_shared<rclcpp::Node>("mtc_node", options) }
{
  // Declare parameter for taught positions file path
  // Priority: 1. SNAP_USER_DATA (snap), 2. ROS_DATA_PATH (env var), 3. workspace data dir
  std::string default_data_path;

  // Check environment variables first (for snap/deployment)
  const char* snap_user_data = std::getenv("SNAP_USER_DATA");
  const char* ros_data_path = std::getenv("ROS_DATA_PATH");

  if (snap_user_data) {
    // Running in snap - use snap's writable data directory
    default_data_path = std::string(snap_user_data) + "/taught_positions.yaml";
  } else if (ros_data_path) {
    // Custom data path specified
    default_data_path = std::string(ros_data_path) + "/taught_positions.yaml";
  } else {
    // Development mode - find workspace root from package location
    try {
      // Get package share directory (e.g., <workspace>/install/share/mtc_demo)
      std::string pkg_share = ament_index_cpp::get_package_share_directory("mtc_demo");
      // Go up 3 levels to workspace root: install/share/mtc_demo -> install/share -> install -> workspace
      std::string workspace_root = pkg_share + "/../../..";
      default_data_path = workspace_root + "/data/taught_positions.yaml";
    } catch (const std::exception& e) {
      // Fallback if package not found
      RCLCPP_WARN(LOGGER, "Could not find package directory: %s", e.what());
      default_data_path = "/tmp/taught_positions.yaml";
    }
  }

  node_->declare_parameter("taught_positions_file", default_data_path);
  RCLCPP_INFO(LOGGER, "Taught positions file path: %s", default_data_path.c_str());

  // Create service for executing MTC task
  execute_task_service_ = node_->create_service<std_srvs::srv::Trigger>(
    "/mtc/execute_task",
    std::bind(&MTCTaskNode::executeTaskCallback, this, std::placeholders::_1, std::placeholders::_2));

  RCLCPP_INFO(LOGGER, "MTC execution service ready");
}

// Getter to get the base interface
rclcpp::node_interfaces::NodeBaseInterface::SharedPtr MTCTaskNode::getNodeBaseInterface()
{
  return node_->get_node_base_interface();
}

// Class Method - Setting up the planning scene
void MTCTaskNode::setupPlanningScene()
{
  moveit_msgs::msg::CollisionObject object;
  object.id = "object";
  object.header.frame_id = "world";
  object.primitives.resize(1);
  object.primitives[0].type = shape_msgs::msg::SolidPrimitive::CYLINDER;
  object.primitives[0].dimensions = { 0.05, 0.02 };

  geometry_msgs::msg::Pose pose;
  pose.position.x = 0.1; // Closer to robot
  pose.position.y = -0.4; // Less to the side
  pose.position.z = 0.15; // Lower for easier top-down reach
  pose.orientation.w = 1.0;
  object.pose = pose;

  moveit::planning_interface::PlanningSceneInterface psi;
  psi.applyCollisionObject(object);
}

// Service callback for executing MTC task
void MTCTaskNode::executeTaskCallback(
  const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
  std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  (void)request;  // Unused parameter

  RCLCPP_INFO(LOGGER, "=== MTC TASK EXECUTION REQUESTED ===");

  try {
    doTask();
    response->success = true;
    response->message = "MTC task executed successfully";
    RCLCPP_INFO(LOGGER, "MTC task completed successfully");
  }
  catch (const std::exception& e) {
    response->success = false;
    response->message = std::string("MTC task failed: ") + e.what();
    RCLCPP_ERROR(LOGGER, "MTC task failed: %s", e.what());
  }
}

// Load taught positions from YAML file
bool MTCTaskNode::loadTaughtPositions(std::map<std::string, double>& pick_joints,
                                       std::map<std::string, double>& place_joints,
                                       std::map<std::string, double>& home_joints,
                                       double& gripper_close_width)
{
  try {
    // Get taught positions file path from parameter
    std::string yaml_path = node_->get_parameter("taught_positions_file").as_string();

    // Check if file exists
    std::ifstream file(yaml_path);
    if (!file.good()) {
      RCLCPP_ERROR(LOGGER, "Taught positions file not found: %s", yaml_path.c_str());
      RCLCPP_ERROR(LOGGER, "Please teach positions first using the joystick");
      return false;
    }

    RCLCPP_INFO(LOGGER, "Loading taught positions from: %s", yaml_path.c_str());

    // Load YAML
    YAML::Node config = YAML::LoadFile(yaml_path);

    // Load pick position
    if (config["pick_position"] && config["pick_position"]["joints"]) {
      for (const auto& joint : config["pick_position"]["joints"]) {
        pick_joints[joint.first.as<std::string>()] = joint.second.as<double>();
      }
      RCLCPP_INFO(LOGGER, "Loaded pick position with %zu joints", pick_joints.size());

      // Load gripper close width from pick position (D-pad configured width)
      if (config["pick_position"]["gripper_close_width"]) {
        gripper_close_width = config["pick_position"]["gripper_close_width"].as<double>();
        RCLCPP_INFO(LOGGER, "Loaded gripper close width: %.1f mm", gripper_close_width * 1000.0);
      } else {
        RCLCPP_WARN(LOGGER, "No gripper_close_width found, using default 0mm (fully closed)");
        gripper_close_width = 0.0;
      }
    } else {
      RCLCPP_ERROR(LOGGER, "pick_position not found in YAML");
      return false;
    }

    // Load place position
    if (config["place_position"] && config["place_position"]["joints"]) {
      for (const auto& joint : config["place_position"]["joints"]) {
        place_joints[joint.first.as<std::string>()] = joint.second.as<double>();
      }
      RCLCPP_INFO(LOGGER, "Loaded place position with %zu joints", place_joints.size());
    } else {
      RCLCPP_ERROR(LOGGER, "place_position not found in YAML");
      return false;
    }

    // Load home position
    if (config["home_position"] && config["home_position"]["joints"]) {
      for (const auto& joint : config["home_position"]["joints"]) {
        home_joints[joint.first.as<std::string>()] = joint.second.as<double>();
      }
      RCLCPP_INFO(LOGGER, "Loaded home position with %zu joints", home_joints.size());
    } else {
      RCLCPP_ERROR(LOGGER, "home_position not found in YAML");
      return false;
    }

    return true;
  }
  catch (const YAML::Exception& e) {
    RCLCPP_ERROR(LOGGER, "YAML parsing error: %s", e.what());
    return false;
  }
  catch (const std::exception& e) {
    RCLCPP_ERROR(LOGGER, "Error loading taught positions: %s", e.what());
    return false;
  }
}

// Class Method - Interfaces with the MoveIt Task Constructor task object
void MTCTaskNode::doTask()
{
  task_ = createTask();

  try
  {
    task_.init();
  }
  catch (mtc::InitStageException& e)
  {
    RCLCPP_ERROR_STREAM(LOGGER, e);
    return;
  }

  if (!task_.plan(5))
  {
    RCLCPP_ERROR_STREAM(LOGGER, "Task planning failed");
    return;
  }

  // Publish solution to be visualized in RViz
  task_.introspection().publishSolution(*task_.solutions().front());

  // Excute the plan - action server interface plugin with Rviz
  auto result = task_.execute(*task_.solutions().front());
  if (result.val != moveit_msgs::msg::MoveItErrorCodes::SUCCESS)
  {
    RCLCPP_ERROR_STREAM(LOGGER, "Task execution failed");
    return;
  }

  return;
}

// Class Method - Creates a MoveIt Task Constructor object and set some properties
mtc::Task MTCTaskNode::createTask()
{
  // Load taught positions
  std::map<std::string, double> pick_joints, place_joints, home_joints;
  double gripper_close_width = 0.0;  // Default: fully closed
  if (!loadTaughtPositions(pick_joints, place_joints, home_joints, gripper_close_width)) {
    RCLCPP_ERROR(LOGGER, "Failed to load taught positions! Using default task.");
    // Fall through to create default task if taught positions not available
  }

  mtc::Task task;
  task.stages()->setName("taught position task");
  task.loadRobotModel(node_);

  const auto& arm_group_name = "kr810_arm";
  const auto& hand_group_name = "rg2_gripper";
  const auto& eef_name = "rg2";  // End effector name from SRDF
  const auto& hand_frame = "rg2_hand";

  // Set task properties
  task.setProperty("group", arm_group_name);
  task.setProperty("eef", eef_name);  // Use end effector name, not group name
  task.setProperty("ik_frame", hand_frame);

  // Add current state (required for MTC)
  auto stage_state_current = std::make_unique<mtc::stages::CurrentState>("current");
  task.add(std::move(stage_state_current));

  // Define the type of robot motor

  //PipelinePlanner - defaults to OMPL, configured with RRTConnect for faster planning
  auto sampling_planner = std::make_shared<mtc::solvers::PipelinePlanner>(node_);
  sampling_planner->setMaxVelocityScalingFactor(0.3);     // 30% speed - very conservative for high-inertia joints
  sampling_planner->setMaxAccelerationScalingFactor(0.3); // 30% acceleration - prevents torque errors
  sampling_planner->setPlannerId("RRTConnect");

  //JoingInterpolation - simpler planner that interpolates from start to goal, used for simple motions
  auto interpolation_planner = std::make_shared<mtc::solvers::JointInterpolationPlanner>();
  interpolation_planner->setMaxVelocityScalingFactor(0.3);
  interpolation_planner->setMaxAccelerationScalingFactor(0.3);

  //CartesianPath - used to move the end effector in a straight line in Cartesian space - USED IN THIS EXAMPLE
  auto cartesian_planner = std::make_shared<mtc::solvers::CartesianPath>();
  cartesian_planner->setMaxVelocityScalingFactor(0.3);     // 30% speed - very conservative for high-inertia joints
  cartesian_planner->setMaxAccelerationScalingFactor(0.3); // 30% acceleration - prevents torque errors
  cartesian_planner->setStepSize(.01);  // Original 10mm - reverted for reliable planning
  cartesian_planner->setJumpThreshold(0.0);  // Original unlimited jumps - reverted
  cartesian_planner->setMinFraction(0.95);  // Original 95%

  // TASK DEFINITION USING TAUGHT POSITIONS //

  // 1. Open hand
  {
    auto stage = std::make_unique<mtc::stages::MoveTo>("open hand", interpolation_planner);
    stage->setGroup(hand_group_name);
    stage->setGoal("open");
    task.add(std::move(stage));
  }

  // 2. Move to taught pick position
  if (!pick_joints.empty()) {
    // Filter out gripper joints - only use arm joints (joint_1 through joint_7)
    std::map<std::string, double> arm_joints_only;
    for (const auto& joint : pick_joints) {
      if (joint.first.find("joint_") == 0) {  // Only arm joints
        arm_joints_only[joint.first] = joint.second;
      }
    }

    if (!arm_joints_only.empty()) {
      auto stage = std::make_unique<mtc::stages::MoveTo>("move to pick position", sampling_planner);
      stage->setGroup(arm_group_name);
      stage->setGoal(arm_joints_only);
      task.add(std::move(stage));
    } else {
      RCLCPP_ERROR(LOGGER, "No arm joints found in pick position!");
    }
  }

  // 3. Close hand to grasp (using D-pad configured width)
  {
    auto stage = std::make_unique<mtc::stages::MoveTo>("close hand", interpolation_planner);
    stage->setGroup(hand_group_name);

    // Use explicit gripper width from D-pad setting instead of SRDF "close" position
    std::map<std::string, double> gripper_goal;
    gripper_goal["left_finger_joint"] = gripper_close_width;
    gripper_goal["right_finger_joint"] = -gripper_close_width;

    RCLCPP_INFO(LOGGER, "Setting gripper close width to %.1f mm", gripper_close_width * 1000.0);
    stage->setGoal(gripper_goal);
    task.add(std::move(stage));
  }

  // 4. Move to taught place position
  if (!place_joints.empty()) {
    // Filter out gripper joints - only use arm joints
    std::map<std::string, double> arm_joints_only;
    for (const auto& joint : place_joints) {
      if (joint.first.find("joint_") == 0) {  // Only arm joints
        arm_joints_only[joint.first] = joint.second;
      }
    }

    if (!arm_joints_only.empty()) {
      auto stage = std::make_unique<mtc::stages::MoveTo>("move to place position", sampling_planner);
      stage->setGroup(arm_group_name);
      stage->setGoal(arm_joints_only);
      task.add(std::move(stage));
    } else {
      RCLCPP_ERROR(LOGGER, "No arm joints found in place position!");
    }
  }

  // 5. Open hand to release
  {
    auto stage = std::make_unique<mtc::stages::MoveTo>("open hand", interpolation_planner);
    stage->setGroup(hand_group_name);
    stage->setGoal("open");
    task.add(std::move(stage));
  }

  // 6. Return to taught home position
  if (!home_joints.empty()) {
    // Filter out gripper joints - only use arm joints (joint_1 through joint_7)
    std::map<std::string, double> arm_joints_only;
    for (const auto& joint : home_joints) {
      if (joint.first.find("joint_") == 0) {  // Only arm joints
        arm_joints_only[joint.first] = joint.second;
      }
    }

    if (!arm_joints_only.empty()) {
      auto stage = std::make_unique<mtc::stages::MoveTo>("return to taught home", sampling_planner);
      stage->setGroup(arm_group_name);
      stage->setGoal(arm_joints_only);
      task.add(std::move(stage));
    } else {
      RCLCPP_ERROR(LOGGER, "No arm joints found in home position!");
    }
  } else {
    // Fallback to SRDF "ready" position if home not taught
    auto stage = std::make_unique<mtc::stages::MoveTo>("return home (ready)", interpolation_planner);
    stage->setGroup(arm_group_name);
    stage->setGoal("ready");
    task.add(std::move(stage));
  }

  return task;
}

int main(int argc, char** argv)
{
  // Initialize the ROS 2 context with command line arguments
  rclcpp::init(argc, argv);

  // Create NodeOptions to configure the behavior of the ROS 2 node
  rclcpp::NodeOptions options;
  options.automatically_declare_parameters_from_overrides(true);

  // Create an instance of the MTCTaskNode class using the provided options
  auto mtc_task_node = std::make_shared<MTCTaskNode>(options);

  // Create a MultiThreadedExecutor for handling multiple threads
  rclcpp::executors::MultiThreadedExecutor executor;

  // Create a separate thread for spinning the executor
  auto spin_thread = std::make_unique<std::thread>([&executor, &mtc_task_node]() {
    // Add the MTCTaskNode to the executor
    executor.add_node(mtc_task_node->getNodeBaseInterface());
    
    // Spin (process callbacks) until shutdown is called
    executor.spin();

    // Remove the MTCTaskNode from the executor when spinning is done
    executor.remove_node(mtc_task_node->getNodeBaseInterface());
  });

  // Call custom setup method on the MTCTaskNode
  mtc_task_node->setupPlanningScene();

  RCLCPP_INFO(LOGGER, "MTC Node ready. Waiting for service calls...");

  // Wait for the spinning thread to finish (keeps node alive for service calls)
  spin_thread->join();

  // Shutdown the ROS 2 context, releasing resources and cleaning up
  rclcpp::shutdown();

  // Return 0 to indicate successful execution
  return 0;
}
