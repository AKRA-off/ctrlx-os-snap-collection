// Copyright 2023 ros2_control Development Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "kr_robot_driver/kr810_hardware.hpp"
#include <chrono>
#include <cmath>
#include <string>
#include <vector>

namespace kr_robot_driver
{

RobotSystem::RobotSystem()
: logger_(rclcpp::get_logger("KR810_Hardware"))
, is_gripper_connected_(false)
, is_connected_(false)
, robot_port_(12321)
, last_gripper_command_(0.025)  // Initialize to open position
, last_sent_gripper_command_(0.025)
, gripper_is_open_(true)
, gripper_update_counter_(0)
{
  RCLCPP_INFO(logger_, "Initializing KR810 Hardware Interface");
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

  // robot has 7 joints + optional gripper (1 actuated joint)
  joint_position_.assign(8, 0.0);  // 7 arm + 1 gripper (left finger only, right is mimic)
  joint_velocities_.assign(8, 0.0);
  joint_position_command_.assign(8, 0.0);
  joint_velocities_command_.assign(8, 0.0);

  // force/torque sensor has 6 readings (3 forces + 3 torques)
  ft_states_.assign(6, 0.0);
  ft_command_.assign(6, 0.0);

  // Build joint interface map
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

  // Check if gripper is configured
  is_gripper_connected_ = (joint_interfaces["position"].size() > 7);
  if (is_gripper_connected_) {
    RCLCPP_INFO(logger_, "Gripper detected in configuration");
  }

  RCLCPP_INFO(logger_, "KR810 Hardware Interface initialized with %zu joints",
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
    // If not connected, just echo commands (simulation mode)
    for (size_t i = 0; i < joint_position_command_.size(); ++i) {
      joint_position_[i] = joint_position_command_[i];
      joint_velocities_[i] = joint_velocities_command_[i];
    }
    return return_type::OK;
  }

  // Read robot state from KORD API
  try {
    // Synchronize with robot and fetch latest state
    if (receiver_interface_->fetchStatus()) {
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

      // Gripper state (if connected)
      // Note: RG2 CBUN via tool connector provides gripper state feedback
      // For basic implementation, we echo the commanded state
      // Right finger automatically follows via URDF mimic joint
      if (is_gripper_connected_) {
        // Echo the commanded position (smooth feedback)
        joint_position_[7] = last_gripper_command_;
        joint_velocities_[7] = 0.0;

        // Send gripper IO commands at lower frequency to avoid packet loss
        // Update every 20 cycles (0.2 seconds at 100 Hz) and only if command changed
        gripper_update_counter_++;
        if (gripper_update_counter_ >= 20) {
          gripper_update_counter_ = 0;

          // Only send if command changed significantly (> 0.005m = 5mm)
          if (std::abs(last_gripper_command_ - last_sent_gripper_command_) > 0.005) {
            controlGripper(last_gripper_command_);
            last_sent_gripper_command_ = last_gripper_command_;
          }
        }
      }
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
    // If not connected, do nothing (simulation mode)
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

    // Send direct position/velocity command to robot
    if (control_interface_) {
      control_interface_->directJControl(q, qd, qdd);
    }

    // Gripper control (if connected) - sent via CBUN tool connector IO
    // Note: Do NOT send IO requests in real-time control loop - causes KORD packet loss
    // Instead, just store the command and send it in read() at lower frequency
    if (is_gripper_connected_) {
      last_gripper_command_ = joint_position_command_[7];
    }

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
    // Create control and receiver interfaces BEFORE connecting (required by KORD API)
    control_interface_ = std::make_shared<kr2::kord::ControlInterface>(kord_core_);
    receiver_interface_ = std::make_shared<kr2::kord::ReceiverInterface>(kord_core_);

    // Connect KORD Core to robot
    // connect() parameter is the network device (NULL = use default)
    if (!kord_core_->connect(NULL)) {
      RCLCPP_ERROR(logger_, "Failed to connect KORD Core to robot");
      return false;
    }

    // CRITICAL: Sync with robot before using directJControl()
    // This initializes the robot's state machine and prepares for real-time control
    RCLCPP_INFO(logger_, "Synchronizing with robot...");
    if (!kord_core_->syncRC()) {  // Uses default F_SYNC_FULL_ROTATION flag
      RCLCPP_ERROR(logger_, "Failed to synchronize with robot");
      return false;
    }
    RCLCPP_INFO(logger_, "Robot synchronized successfully");

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

void RobotSystem::openGripper()
{
#ifdef ENABLE_REAL_ROBOT
  if (!is_connected_ || !is_gripper_connected_) {
    return;
  }

  try {
    // Create IO request to open gripper
    // For RG2 via CBUN tool connector: TB1=high, TB2=low opens the gripper
    kr2::kord::RequestIO io_request;
    io_request.asSetIODigitalOut()
              .withEnabledPorts(kr2::kord::RequestIO::DIGITAL_IOTOOLB::TB1)
              .withDisabledPorts(kr2::kord::RequestIO::DIGITAL_IOTOOLB::TB2);

    // Send the request
    if (control_interface_) {
      control_interface_->transmitRequest(io_request);
      gripper_is_open_ = true;
      RCLCPP_DEBUG(logger_, "Gripper opened (TB1=high, TB2=low)");
    }
  } catch (const std::exception & e) {
    RCLCPP_ERROR(logger_, "Failed to open gripper: %s", e.what());
  }
#endif
}

void RobotSystem::closeGripper()
{
#ifdef ENABLE_REAL_ROBOT
  if (!is_connected_ || !is_gripper_connected_) {
    return;
  }

  try {
    // Create IO request to close gripper
    // For RG2 via CBUN tool connector: TB1=low, TB2=high closes the gripper
    kr2::kord::RequestIO io_request;
    io_request.asSetIODigitalOut()
              .withDisabledPorts(kr2::kord::RequestIO::DIGITAL_IOTOOLB::TB1)
              .withEnabledPorts(kr2::kord::RequestIO::DIGITAL_IOTOOLB::TB2);

    // Send the request
    if (control_interface_) {
      control_interface_->transmitRequest(io_request);
      gripper_is_open_ = false;
      RCLCPP_DEBUG(logger_, "Gripper closed (TB1=low, TB2=high)");
    }
  } catch (const std::exception & e) {
    RCLCPP_ERROR(logger_, "Failed to close gripper: %s", e.what());
  }
#endif
}

void RobotSystem::controlGripper(double position)
{
  // Threshold to determine if gripper should be open or closed
  // Based on SRDF: open=0.025, close=0.01
  const double GRIPPER_THRESHOLD = 0.0175;  // Midpoint between open and close

  bool should_be_open = (position > GRIPPER_THRESHOLD);

  // Only send command if state changes to avoid spamming IO requests
  if (should_be_open != gripper_is_open_) {
    if (should_be_open) {
      openGripper();
    } else {
      closeGripper();
    }
    last_gripper_command_ = position;
  }
}

}  // namespace kr_robot_driver

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  kr_robot_driver::RobotSystem, hardware_interface::SystemInterface)
