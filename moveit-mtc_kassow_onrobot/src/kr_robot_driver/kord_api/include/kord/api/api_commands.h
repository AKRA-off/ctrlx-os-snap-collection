/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2025, Kassow Robots
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Kassow Robots nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#ifndef KR2_KORD_API_COMMANDS_H
#define KR2_KORD_API_COMMANDS_H

#include <array>
#include <kord/protocol/JointFirmwareCommand.h>

namespace kr2::kord {

/**
 * @struct RobotArmStatus
 * @brief Holds status variables related to the robot arm.
 *
 * This structure encapsulates various status indicators of the robot arm,
 * including hardware states, software states, channel quality, and transmission
 * statistics. It reflects the content of the Status frame and captures dynamic
 * variables related to joint movement and hardware. Additionally, it stores
 * transmission statistics such as jitter, delay, and API performance metrics.
 */
struct RobotArmStatus {
    /** Current positions of the robot arm's joints */
    std::array<double, 7UL> positions_{};

    /** Current speeds of the robot arm's joints */
    std::array<double, 7UL> speed_{};

    /** Current accelerations of the robot arm's joints */
    std::array<double, 7UL> accelerations_{};

    /** Current torques of the robot arm's joints */
    std::array<double, 7UL> torques_{};

    /** Current temperatures of the robot arm's joints */
    std::array<double, 7UL> temperatures_{};

    /** Error bits indicating various error states of the robot arm */
    std::array<double, 7UL> error_bits_{};

    /** Status bits indicating various operational states of the robot arm */
    std::array<double, 7UL> status_bits_{};

    /** TCP (Tool Center Point) model data */
    std::array<double, 6UL> tcp_model_{};

    /** TCP sensor data */
    std::array<double, 6UL> tcp_sensor_{};

    /** Number of failed attempts to read empty frames */
    uint32_t fail_to_read_empty{};

    /** Number of failed attempts to read frames with errors */
    uint32_t fail_to_read_error{};

    /** Minimum tick delay recorded */
    int64_t min_tick_delay{};

    /** Maximum tick delay recorded */
    int64_t max_tick_delay{};

    /** Average tick delay recorded */
    int64_t average_tick_delay{};

    /** Minimum delay recorded */
    int64_t min_delay{};

    /** Maximum delay recorded */
    int64_t max_delay{};

    /** Average delay recorded */
    int64_t average_delay{};

    /** Number of faulty frames that have started */
    int64_t faulty_frames_start{};

    /** Maximum number of frames processed in a single tick */
    size_t max_frames_in_tick{};

    /** Safety flags indicating safety-related states */
    unsigned int rc_safety_flags_{};

    /** Motion flags indicating motion-related states */
    unsigned int rc_motion_flags_{};
};

/**
 * @struct RobotArmCommand
 * @brief Represents commands to control the robot arm.
 *
 * This structure encapsulates various command parameters required to
 * control the robot arm's movements and firmware interactions. It includes
 * the type of command, sequence number, target positions, speeds, accelerations,
 * and firmware commands for each joint.
 */
struct RobotArmCommand {

    /**
     * @enum EType
     * @brief Enumerates the types of commands that can be issued to the robot arm.
     */
    enum EType {
        eInvalid = 0, /**< Represents an invalid or uninitialized command type */
        eMOVE,        /**< Command to move the robot arm to specified positions */
        eDJC,         /**< Dynamic Joint Control command */
        eFW           /**< Firmware-related command */
    };

    /** Type of the command */
    EType command_type = eInvalid;

    /** Sequence number of the command, used for tracking and synchronization */
    unsigned int seq_{};

    /** Target positions for each of the robot arm's joints */
    std::array<double, 7UL> positions_{};

    /** Target speeds for each of the robot arm's joints */
    std::array<double, 7UL> speed_{};

    /** Target accelerations for each of the robot arm's joints */
    std::array<double, 7UL> accelerations_{};

    /** Firmware commands for each joint, defined in the protocol */
    std::array<protocol::EJointFirmwareCommand, 7UL> fw_cmds_{};
};

/**
 * @struct RobotFrameCommand
 * @brief Represents a frame command for the robot arm.
 *
 * This structure defines the parameters required to issue frame-based commands
 * to the robot arm, such as moving to a target pose, setting target velocities,
 * and configuring accelerations. It includes methods to configure the type of
 * control and to set various target parameters.
 */
struct RobotFrameCommand {
    /**
     * @brief Enumerates the types of frame commands available.
     */
    enum {
        MOVE_POSE,       /**< Command to move the robot arm to a specific pose */
        MOVE_VELOCITY,   /**< Command to move the robot arm with a specific velocity */
        MOVE_POSE_DYN    /**< Dynamic pose control command */
    };

    /** Type of the frame command */
    unsigned int command_type;

    /** Sequence number of the frame command, used for tracking and synchronization */
    unsigned int seq_;

    /** Target pose for the Tool Center Point (TCP) of the robot arm */
    std::array<double, 6UL> tcp_target_;

    /** Target velocity for the TCP of the robot arm */
    std::array<double, 6UL> tcp_target_velocity_;

    /** Target acceleration for the TCP of the robot arm */
    std::array<double, 6UL> tcp_target_acc_;

    /**
     * @brief Configures the command as a pose control command.
     *
     * This method sets the command type to `MOVE_POSE`, indicating that the
     * robot arm should move to a specified pose.
     *
     * @return Reference to the modified `RobotFrameCommand` object.
     */
    RobotFrameCommand &asPoseControl();

    /**
     * @brief Configures the command as a velocity control command.
     *
     * This method sets the command type to `MOVE_VELOCITY`, indicating that the
     * robot arm should move with a specified velocity.
     *
     * @return Reference to the modified `RobotFrameCommand` object.
     */
    RobotFrameCommand &asVelocityControl();

    /**
     * @brief Configures the command as a position and velocity control command.
     *
     * This method sets the command type to `MOVE_POSE_DYN`, indicating that the
     * robot arm should move to a specified pose with dynamic velocity control.
     *
     * @return Reference to the modified `RobotFrameCommand` object.
     */
    RobotFrameCommand &asPosVelControl();

    /**
     * @brief Sets the target pose for the robot arm's TCP.
     *
     * @param target_pose An array of 6 doubles representing the target pose coordinates.
     * @return Reference to the modified `RobotFrameCommand` object.
     */
    RobotFrameCommand &withTargetPose(const std::array<double, 6UL> &target_pose);

    /**
     * @brief Sets the target velocity for the robot arm's TCP.
     *
     * @param target_velocity An array of 6 doubles representing the target velocity.
     * @return Reference to the modified `RobotFrameCommand` object.
     */
    RobotFrameCommand &withTargetVelocity(const std::array<double, 6UL> &target_velocity);

    /**
     * @brief Sets the target acceleration for the robot arm's TCP.
     *
     * @param target_acc An array of 6 doubles representing the target acceleration.
     * @return Reference to the modified `RobotFrameCommand` object.
     */
    RobotFrameCommand &withTargetAcceleration(const std::array<double, 6UL> &target_acc);

    /**
     * @brief Sets the sequence number for the frame command.
     *
     * @param seq The sequence number to assign to the command.
     * @return Reference to the modified `RobotFrameCommand` object.
     */
    RobotFrameCommand &setSequenceNumber(unsigned int seq);
};

} // namespace kr2::kord

#endif // KR2_KORD_API_COMMANDS_H

