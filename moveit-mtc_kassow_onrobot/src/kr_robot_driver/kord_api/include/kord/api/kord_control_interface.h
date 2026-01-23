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

#ifndef KR2_KORD_CONTROL_INTERFACE_H
#define KR2_KORD_CONTROL_INTERFACE_H

#include <memory>
#include <vector>

#include <kord/api/api_request.h>
#include <kord/api/kord.h>

namespace kr2::kord {

/**
 * @typedef token_t
 * @brief Type definition for tokens used in command tracking.
 */
typedef int64_t token_t;

class KordCore;

/**
 * @class ControlInterface
 * @brief Interface for controlling the robot arm in both joint and linear spaces.
 *
 * The ControlInterface provides methods to move the robot arm by specifying joint configurations
 * or linear poses. It supports various movement commands, brake controls, direct control modes,
 * and alarm management. Future versions will include additional control functionalities.
 */
class ControlInterface {
public:
    /**
     * @brief Constructs a new ControlInterface object.
     *
     * Initializes the ControlInterface with a shared instance of KordCore for communication.
     *
     * @param kord Shared pointer to an instance of the \b KordCore class, providing the communication interface.
     */
    explicit ControlInterface(std::shared_ptr<kord::KordCore> kord);

    /**
     * @brief Destroys the ControlInterface object.
     */
    ~ControlInterface();

    /**
     * @brief Commands the robot to move in joint space by specifying target joint configurations.
     *
     * This method sends a move command in joint space, providing reference joint angles in radians.
     *
     * @note This command operates in real-time.
     *
     * @param q Reference joint configuration to assume at the end of the defined time.
     * @param tt Tracking type.
     * @param tt_value Duration of the movement in seconds if tracking type is time-based, otherwise maximum speed.
     * @param bt Blend type.
     * @param bt_value Blend setting of the target point. Start blending at the defined time if blending type is
     * time-based.
     * @param ot Overlay type.
     * @param sync_time Synchronization time for emitting the sync event. Used internally.
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool moveJ(const std::array<double, 7UL> &q,
               const TrackingType &tt,
               const double &tt_value,
               const BlendType &bt,
               const double &bt_value,
               const OverlayType &ot,
               const double &sync_time = std::numeric_limits<double>::infinity());

    /**
     * @brief Commands the robot to move in linear space by specifying target TCP pose.
     *
     * This method sends a move command in linear space, providing reference TCP pose in meters.
     *
     * @note This command operates in real-time.
     *
     * @param p Reference TCP pose [x, y, z, r, p, y] to assume at the end of the defined time.
     * @param tt Tracking type.
     * @param tt_value Duration of the movement in seconds if tracking type is time-based, otherwise maximum speed.
     * @param bt Blend type.
     * @param bt_value Blend setting of the target point. Start blending at the defined time.
     * @param ot Overlay type.
     * @param sync_time Synchronization time for emitting the sync event. Used internally.
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool moveL(const std::array<double, 6UL> &p,
               const TrackingType &tt,
               const double &tt_value,
               const BlendType &bt,
               const double &bt_value,
               const OverlayType &ot,
               const double &sync_time = std::numeric_limits<double>::infinity());

    /**
     * @brief Commands the robot to move in linear space by specifying target TCP pose with quaternion.
     *
     * This method sends a move command in linear space, providing reference TCP pose with quaternion
     * for orientation.
     *
     * @note This command operates in real-time.
     *
     * @param pose_quat Reference TCP pose [x, y, z, w, x, y, z] to assume at the end of the defined time.
     *                  The quaternion is provided as [w, x, y, z].
     * @param tt Tracking type.
     * @param tt_value Duration of the movement in seconds if tracking type is time-based, otherwise maximum speed.
     * @param bt Blend type.
     * @param bt_value Blend setting of the target point. Start blending at the defined time.
     * @param ot Overlay type.
     * @param sync_time Synchronization time for emitting the sync event. Used internally.
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool moveL(const std::array<double, 7UL> &pose_quat,
               const TrackingType &tt,
               const double &tt_value,
               const BlendType &bt,
               const double &bt_value,
               const OverlayType &ot,
               const double &sync_time = std::numeric_limits<double>::infinity());

    /**
     * @brief Commands the robot to move with specified velocities in linear space.
     *
     * This method sends a velocity-based move command in linear space.
     *
     * @note This command operates in real-time.
     *
     * @param v Reference TCP velocity [x, y, z, r, p, y] to assume at the end of the defined time.
     * @param sync_value Unused parameter.
     * @param period Duration of the movement.
     * @param timeout Time between the end of the movement and full stop (zero velocity).
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool moveV(const std::array<double, 6UL> &v, long sync_value, double period, double timeout);

    /**
     * @brief Uses self-motion to move the robot while keeping its head position.
     *
     * This method enables self-motion control, allowing the robot to move autonomously while maintaining its head
     * position.
     *
     * @note This command operates in real-time.
     *
     * @param manifold_joint_speed Speed of joints to use during self-motion control.
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool moveManifold(double manifold_joint_speed);

    /**
     * @brief Sets new frame coordinates for the robot.
     *
     * This method updates the frame of reference for the robot's TCP pose.
     *
     * @note This command operates in real-time.
     *
     * @param frame_id Identifier for the frame of reference (EFrameID).
     * @param pose Rotation and position coordinates [x, y, z, r, p, y].
     * @param ref_frame Position reference, World Frame by default (EFrameValue).
     * @param token Token of the sent command, populated token can then be used by getCommandStatus(token).
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool setFrame(EFrameID frame_id, std::array<double, 6> pose, EFrameValue ref_frame, token_t &token);

    /**
     * @brief Sets new frame coordinates for the robot without using a token.
     *
     * This method updates the frame of reference for the robot's TCP pose.
     *
     * @param frame_id Identifier for the frame of reference (EFrameID).
     * @param pose Rotation and position coordinates [x, y, z, r, p, y].
     * @param ref_frame Position reference, World Frame by default (EFrameValue).
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool setFrame(EFrameID frame_id, std::array<double, 6> pose, EFrameValue ref_frame);

    /**
     * @brief Sets specified load parameters for the robot.
     *
     * This method updates the robot's load parameters including mass, center of gravity (CoG), and inertia.
     *
     * @param load_id Identifier for the load parameters (ELoadID).
     * @param mass Mass of the load.
     * @param cog Center of Gravity as an array [x, y, z].
     * @param inertia Inertia as an array [xx, yy, zz, xy, xz, yz].
     * @param token Token of the sent command, populated token can then be used by getCommandStatus(token).
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool setLoad(ELoadID load_id, double mass, std::array<double, 3> cog, std::array<double, 6> inertia, token_t &token);

    /**
     * @brief Sets specified load parameters for the robot without using a token.
     *
     * This method updates the robot's load parameters including mass, center of gravity (CoG), and inertia.
     *
     * @param load_id Identifier for the load parameters (ELoadID).
     * @param mass Mass of the load.
     * @param cog Center of Gravity as an array [x, y, z].
     * @param inertia Inertia as an array [xx, yy, zz, xy, xz, yz].
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool setLoad(ELoadID load_id, double mass, std::array<double, 3> cog, std::array<double, 6> inertia);

    // Miscellaneous/Experimental control

    /**
     * @brief Attempts to engage brakes on the specified joints.
     *
     * This method engages the brakes on the listed joints, which should be in the range of 1 to 7.
     *
     * @param b Vector of joint indices to engage brakes (1-7).
     * @param token Token of the sent command, populated token can then be used by getCommandStatus(token).
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool engageBrakes(const std::vector<int> &b, token_t &token);

    /**
     * @brief Attempts to engage brakes on the specified joints without using a token.
     *
     * This method engages the brakes on the listed joints, which should be in the range of 1 to 7.
     *
     * @param b Vector of joint indices to engage brakes (1-7).
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool engageBrakes(const std::vector<int> &b);

    /**
     * @brief Attempts to disengage brakes on the specified joints.
     *
     * This method disengages the brakes on the listed joints, which should be in the range of 1 to 7.
     *
     * @param b Vector of joint indices to disengage brakes (1-7).
     * @param token Token of the sent command, populated token can then be used by getCommandStatus(token).
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool disengageBrakes(const std::vector<int> &b, token_t &token);

    /**
     * @brief Attempts to disengage brakes on the specified joints without using a token.
     *
     * This method disengages the brakes on the listed joints, which should be in the range of 1 to 7.
     *
     * @param b Vector of joint indices to disengage brakes (1-7).
     * @return true Command is successfully sent.
     * @return false Command is not sent.
     */
    bool disengageBrakes(const std::vector<int> &b);

    /**
     * @brief [Warning: Experimental Feature] Uses direct joint control with torque.
     *
     * The commands are directly translated into frames and sent to the robot arm.
     * Use only if you are familiar with the underlying control mechanisms.
     *
     * @note This command operates in real-time.
     *
     * @param q Target joint positions.
     * @param qd Target joint speeds.
     * @param qdd Target joint accelerations.
     * @return true Frame was created, added to the transmission batch, and sent.
     * @return false Frame was not sent.
     */
    bool directJControl(const std::array<double, 7UL> &q,
                        const std::array<double, 7UL> &qd,
                        const std::array<double, 7UL> &qdd);

    /**
     * @brief [Warning: Experimental Feature] Uses direct joint control with torque.
     *
     * The commands are directly translated into frames and sent to the robot arm.
     * Use only if you are familiar with the underlying control mechanisms.
     *
     * @note This command operates in real-time.
     *
     * @param q Target joint positions.
     * @param qd Target joint speeds.
     * @param qdd Target joint accelerations.
     * @param torque Target joint torques.
     * @return true Frame was created, added to the transmission batch, and sent.
     * @return false Frame was not sent.
     */
    bool directJControl(const std::array<double, 7UL> &q,
                        const std::array<double, 7UL> &qd,
                        const std::array<double, 7UL> &qdd,
                        const std::array<double, 7UL> &torque);

    /**
     * @brief [Warning: Experimental Feature] Uses direct torque control.
     *
     * The commands are directly translated into frames and sent to the robot arm.
     * Use only if you are familiar with the underlying control mechanisms.
     *
     * @note This command operates in real-time.
     *
     * @param torque Target joint torques.
     * @return true Frame was created, added to the transmission batch, and sent.
     * @return false Frame was not sent.
     */
    bool directTorqueControl(const std::array<double, 7UL> &torque);

    /**
     * @enum EClearRequest
     * @brief Enumeration representing different types of alarm clearing requests.
     */
    enum EClearRequest {
        CLEAR_HALT = 0,    /**< Attempts to clear the HALT state. */
        UNSUSPEND = 1,     /**< Attempts to unsuspend the robot. */
        CONTINUE_INIT = 2, /**< Attempts to reinitialize the robot if initialization is halted. */
        RESUME = 3,        /**< Resumes paused movement. */
        CBUN_EVENT = 4     /**< Clears CBun event. */
    };

    /**
     * @brief Clears specified robot alarm states.
     *
     * This method sends a request to clear specific alarm states based on the provided alarm request type.
     *
     * @param alarm_request Type of alarm clearing request (EClearRequest).
     * @return A valid token if the request is successfully sent; 0 otherwise.
     */
    int64_t clearAlarmRequest(EClearRequest alarm_request);

    /**
     * @brief Sends a new API request to the remote target.
     *
     * This method transmits a generic API request to the remote controller.
     *
     * @param request The \b Request object containing the API request details.
     * @return A valid token if the request is successfully sent; 0 otherwise.
     */
    token_t transmitRequest(const Request &request);

private:
    /**
     * @struct Internals
     * @brief Internal implementation details of ControlInterface.
     *
     * This structure encapsulates the internal state and implementation details,
     * adhering to the Pimpl (Pointer to Implementation) idiom for encapsulation.
     */
    struct Internals;
    std::shared_ptr<Internals> his_; /**< Shared pointer to the internal implementation details. */
};

} // namespace kr2::kord

#endif // KR2_KORD_CONTROL_INTERFACE_H
