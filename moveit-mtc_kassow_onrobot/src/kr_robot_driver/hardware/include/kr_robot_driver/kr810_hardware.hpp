// Licensed under the Apache License, Version 2.0 (the "License");
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0

#ifndef KR_ROBOT_DRIVER_KR810_HARDWARE_HPP_
#define KR_ROBOT_DRIVER_KR810_HARDWARE_HPP_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/node_interfaces/lifecycle_node_interface.hpp"

// Enable real robot support with KORD API
#ifdef ENABLE_REAL_ROBOT
#include "kord/api/kord.h"
#include "kord/api/kord_control_interface.h"
#include "kord/api/kord_receive_interface.h"
#include "kord/api/kord_io_request.h"
#endif

using hardware_interface::return_type;

namespace kr_robot_driver
{
using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class RobotSystem : public hardware_interface::SystemInterface
{
public:
  RobotSystem();
  ~RobotSystem() override;

  CallbackReturn on_init(const hardware_interface::HardwareInfo & info) override;

  CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override;

  CallbackReturn on_activate(const rclcpp_lifecycle::State & previous_state) override;

  CallbackReturn on_deactivate(const rclcpp_lifecycle::State & previous_state) override;

  std::vector<hardware_interface::StateInterface> export_state_interfaces() override;

  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

  return_type read(const rclcpp::Time & time, const rclcpp::Duration & period) override;

  return_type write(const rclcpp::Time & time, const rclcpp::Duration & period) override;

protected:
  // Robot state data
  std::vector<double> joint_position_command_;
  std::vector<double> joint_velocities_command_;
  std::vector<double> joint_position_;
  std::vector<double> joint_velocities_;
  std::vector<double> ft_states_;
  std::vector<double> ft_command_;

  std::unordered_map<std::string, std::vector<std::string>> joint_interfaces = {
    {"position", {}}, {"velocity", {}}};

#ifdef ENABLE_REAL_ROBOT
  // KORD API objects
  std::shared_ptr<kr2::kord::KordCore> kord_core_;
  std::shared_ptr<kr2::kord::ControlInterface> control_interface_;
  std::shared_ptr<kr2::kord::ReceiverInterface> receiver_interface_;
#endif

  // Connection parameters
  std::string robot_ip_;
  int robot_port_;

  // Robot configuration
  bool is_connected_;

  // Synchronization state
  int sync_failure_count_;
  static const int MAX_SYNC_FAILURES = 10;  // 100ms of failures at 100Hz
  std::shared_ptr<rclcpp::Clock> clock_;

  // Logger
  rclcpp::Logger logger_;

  // Helper methods
  bool connectToRobot();
  void disconnectFromRobot();
  bool isConnected() const;
};

}  // namespace kr_robot_driver

#endif  // KR_ROBOT_DRIVER_KR810_HARDWARE_HPP_
