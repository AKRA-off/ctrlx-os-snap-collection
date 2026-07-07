// Licensed under the Apache License, Version 2.0 (the "License");
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0


#include "kr_robot_driver/kr810_hardware.hpp"
#include <chrono>
#include <cmath>
#include <string>
#include <vector>

namespace kr_robot_driver
{

RobotSystem::RobotSystem()
: logger_(rclcpp::get_logger("KR810_Hardware"))
, is_connected_(false)
, robot_port_(12321)
, sync_failure_count_(0)
, clock_(std::make_shared<rclcpp::Clock>(RCL_ROS_TIME))
{
  RCLCPP_INFO(logger_, "Initializing KR810 Hardware Interface");
  RCLCPP_INFO(logger_, "Note: Gripper controlled via separate TCP controller, not hardware interface");
}

RobotSystem::~RobotSystem()
{
  if (is_connected_) {
    disconnectFromRobot();
  }
}

CallbackReturn RobotSystem::on_init(const hardware_interface::HardwareInfo & info)
{
  if (hardware_interface::SystemInterface::on_init(info) != CallbackReturn::SUCCESS)
  {
    return CallbackReturn::ERROR;
  }

  RCLCPP_INFO(logger_, "Configuring KR810 Hardware Interface");

  // 7 arm joints (gripper controlled separately via TCP)
  joint_position_.assign(7, 0.0);
  joint_velocities_.assign(7, 0.0);
  joint_position_command_.assign(7, 0.0);
  joint_velocities_command_.assign(7, 0.0);

  // 6 readings (3 forces + 3 torques)
  ft_states_.assign(6, 0.0);
  ft_command_.assign(6, 0.0);

  // Joint interface map
  for (const auto & joint : info_.joints)
  {
    for (const auto & interface : joint.state_interfaces)
    {
      joint_interfaces[interface.name].push_back(joint.name);
    }
  }

  // Read robot IP from hardware parameters
  if (info_.hardware_parameters.find("robot_ip") != info_.hardware_parameters.end())
  {
    robot_ip_ = info_.hardware_parameters.at("robot_ip");
    RCLCPP_INFO(logger_, "Robot IP set to: %s", robot_ip_.c_str());
  }
  else
  {
    robot_ip_ = "192.168.2.10";  // Default IP
    RCLCPP_WARN(logger_, "No robot_ip parameter found, using default: %s", robot_ip_.c_str());
  }

  // Read robot port if specified
  if (info_.hardware_parameters.find("robot_port") != info_.hardware_parameters.end())
  {
    robot_port_ = std::stoi(info_.hardware_parameters.at("robot_port"));
  }

  RCLCPP_INFO(logger_, "KR810 Hardware Interface initialized with %zu arm joints (gripper via TCP)",
              joint_interfaces["position"].size());

  return CallbackReturn::SUCCESS;
}

CallbackReturn RobotSystem::on_configure(const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(logger_, "Configuring KR810 Hardware Interface");

  // Initialize KORD Core
  // Parameters: hostname, port, session_id, connection_type
  // session_id = 0 (default), connection_type = UDP_CLIENT
  try {
    kord_core_ = std::make_shared<kr2::kord::KordCore>(
      robot_ip_, robot_port_, 0, kr2::kord::UDP_CLIENT);
    RCLCPP_INFO(logger_, "KORD Core created for %s:%d", robot_ip_.c_str(), robot_port_);
  } catch (const std::exception & e) {
    RCLCPP_ERROR(logger_, "Failed to create KORD Core: %s", e.what());
    return CallbackReturn::ERROR;
  }

  return CallbackReturn::SUCCESS;
}

CallbackReturn RobotSystem::on_activate(const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(logger_, "Activating KR810 Hardware Interface");

  // Connect to robot
  if (!connectToRobot()) {
    RCLCPP_ERROR(logger_, "Failed to connect to robot");
    return CallbackReturn::ERROR;
  }

  RCLCPP_INFO(logger_, "KR810 Hardware Interface activated successfully");
  return CallbackReturn::SUCCESS;
}

CallbackReturn RobotSystem::on_deactivate(const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(logger_, "Deactivating KR810 Hardware Interface");

  // Disconnect from robot
  disconnectFromRobot();

  RCLCPP_INFO(logger_, "KR810 Hardware Interface deactivated");
  return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> RobotSystem::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;

  // Export joint position state interfaces
  int ind = 0;
  for (const auto & joint_name : joint_interfaces["position"])
  {
    state_interfaces.emplace_back(joint_name, "position", &joint_position_[ind++]);
  }

  // Export joint velocity state interfaces
  ind = 0;
  for (const auto & joint_name : joint_interfaces["velocity"])
  {
    state_interfaces.emplace_back(joint_name, "velocity", &joint_velocities_[ind++]);
  }

  // Export force/torque sensor state interfaces
  state_interfaces.emplace_back("tcp_fts_sensor", "force.x", &ft_states_[0]);
  state_interfaces.emplace_back("tcp_fts_sensor", "force.y", &ft_states_[1]);
  state_interfaces.emplace_back("tcp_fts_sensor", "force.z", &ft_states_[2]);
  state_interfaces.emplace_back("tcp_fts_sensor", "torque.x", &ft_states_[3]);
  state_interfaces.emplace_back("tcp_fts_sensor", "torque.y", &ft_states_[4]);
  state_interfaces.emplace_back("tcp_fts_sensor", "torque.z", &ft_states_[5]);

  RCLCPP_INFO(logger_, "Exported %zu state interfaces", state_interfaces.size());
  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> RobotSystem::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;

  // Export joint position command interfaces
  int ind = 0;
  for (const auto & joint_name : joint_interfaces["position"])
  {
    command_interfaces.emplace_back(joint_name, "position", &joint_position_command_[ind++]);
  }

  // Export joint velocity command interfaces
  ind = 0;
  for (const auto & joint_name : joint_interfaces["velocity"])
  {
    command_interfaces.emplace_back(joint_name, "velocity", &joint_velocities_command_[ind++]);
  }

  RCLCPP_INFO(logger_, "Exported %zu command interfaces", command_interfaces.size());
  return command_interfaces;
}

return_type RobotSystem::read(const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  if (!is_connected_) {
    // If not connected, just echo commands
    for (size_t i = 0; i < joint_position_command_.size(); ++i) {
      joint_position_[i] = joint_position_command_[i];
      joint_velocities_[i] = joint_velocities_command_[i];
    }
    return return_type::OK;
  }

  // Read robot state from KORD API
  try {
    // Fetch latest state from capture buffer
    // Note: waitSync() is called in write() before sending commands
    receiver_interface_->fetchData();

    // Get joint positions and velocities
    kr2::kord::KordCore::RobotArmStatus arm_status;
    kord_core_->getRecentArmStatus(arm_status);

    // Update joint states (7 arm joints)
    for (size_t i = 0; i < 7; ++i) {
      joint_position_[i] = arm_status.positions_[i];
      joint_velocities_[i] = arm_status.speed_[i];
    }

    // Update force/torque sensor data
    for (size_t i = 0; i < 6; ++i) {
      ft_states_[i] = arm_status.tcp_sensor_[i];
    }
  } catch (const std::exception & e) {
    RCLCPP_ERROR_THROTTLE(logger_, *rclcpp::Clock::make_shared(), 1000,
                          "Error reading robot state: %s", e.what());
    return return_type::ERROR;
  }

  return return_type::OK;
}

return_type RobotSystem::write(const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  if (!is_connected_) {
    // If not connected, do nothing
    return return_type::OK;
  }

  // Send commands to robot via KORD API
  try {
    // Prepare joint position and velocity commands for arm (7 joints)
    std::array<double, 7> q, qd, qdd;
    for (size_t i = 0; i < 7; ++i) {
      q[i] = joint_position_command_[i];
      qd[i] = joint_velocities_command_[i];
      qdd[i] = 0.0;  // No acceleration control for now
    }

    // Wait for robot heartbeat (4ms cycle) before sending command
    // This synchronizes our 10ms ROS2 cycle with robot's real-time loop
    if (!kord_core_->waitSync(std::chrono::milliseconds(10))) {
      sync_failure_count_++;
      RCLCPP_WARN_THROTTLE(logger_, *clock_, 1000,
          "waitSync timeout in write() - consecutive failures: %d",
          sync_failure_count_);

      // > 100ms of timeouts
      if (sync_failure_count_ > MAX_SYNC_FAILURES) {
        RCLCPP_ERROR(logger_,
            "Persistent sync failures (%d consecutive), marking as disconnected",
            sync_failure_count_);
        is_connected_ = false;
        return return_type::ERROR;
      }
    } else {
      // Reset failure counter on successful sync
      sync_failure_count_ = 0;
    }

    // Send direct position/velocity command to robot
    // Synchronized with robot's 4ms heartbeat for optimal real-time performance
    if (control_interface_) {
      control_interface_->directJControl(q, qd, qdd);
    }

    // Note: Gripper controlled separately via TCP controller (onrobot_tcp_controller.py)

  } catch (const std::exception & e) {
    RCLCPP_ERROR_THROTTLE(logger_, *rclcpp::Clock::make_shared(), 1000,
                          "Error writing robot commands: %s", e.what());
    return return_type::ERROR;
  }

  return return_type::OK;
}

bool RobotSystem::connectToRobot()
{
  RCLCPP_INFO(logger_, "Connecting to Kassow robot at %s:%d", robot_ip_.c_str(), robot_port_);

  try {
    // Create control and receiver interfaces BEFORE connecting
    control_interface_ = std::make_shared<kr2::kord::ControlInterface>(kord_core_);
    receiver_interface_ = std::make_shared<kr2::kord::ReceiverInterface>(kord_core_);

    // Connect KORD Core to robot
    // connect() parameter is the network device (NULL = use default)
    if (!kord_core_->connect(NULL)) {
      RCLCPP_ERROR(logger_, "Failed to connect KORD Core to robot");
      return false;
    }

    // Sync with robot before using directJControl()
    RCLCPP_INFO(logger_, "Synchronizing with robot...");
    if (!kord_core_->syncRC()) {  // Uses default F_SYNC_FULL_ROTATION flag
      RCLCPP_ERROR(logger_, "Failed to synchronize with robot");
      return false;
    }
    RCLCPP_INFO(logger_, "Robot synchronized successfully");

    // Reset sync failure counter on new connection
    sync_failure_count_ = 0;

    // Wait for robot to be ready
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Read initial robot state
    if (receiver_interface_->fetchStatus()) {
      kr2::kord::KordCore::RobotArmStatus arm_status;
      kord_core_->getRecentArmStatus(arm_status);

      // Initialize joint states with current robot position
      for (size_t i = 0; i < 7; ++i) {
        joint_position_[i] = arm_status.positions_[i];
        joint_position_command_[i] = arm_status.positions_[i];
        joint_velocities_[i] = arm_status.speed_[i];
        joint_velocities_command_[i] = 0.0;
      }

      RCLCPP_INFO(logger_, "Initial robot position read successfully");
    }

    is_connected_ = true;
    RCLCPP_INFO(logger_, "Successfully connected to Kassow robot");
    return true;

  } catch (const std::exception & e) {
    RCLCPP_ERROR(logger_, "Exception during robot connection: %s", e.what());
    return false;
  }
}

void RobotSystem::disconnectFromRobot()
{
  if (!is_connected_) {
    return;
  }

  RCLCPP_INFO(logger_, "Disconnecting from Kassow robot");

  try {
    if (kord_core_) {
      kord_core_->disconnect();
    }

    control_interface_.reset();
    receiver_interface_.reset();

    is_connected_ = false;
    RCLCPP_INFO(logger_, "Disconnected from robot");

  } catch (const std::exception & e) {
    RCLCPP_ERROR(logger_, "Exception during robot disconnection: %s", e.what());
  }
}

bool RobotSystem::isConnected() const
{
  return is_connected_;
}


} 

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  kr_robot_driver::RobotSystem, hardware_interface::SystemInterface)
