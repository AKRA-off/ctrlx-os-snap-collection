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

#ifndef KR2_KORD_CORE_H
#define KR2_KORD_CORE_H

#include <map>
#include <string>
#include <vector>

#include <kord/api/api_request.h>
#include <kord/api/connection_interface.h>
#include <kord/api/kord_io_request.h>
#include <kord/protocol/ContentFrameBuilder.h>
#include <kord/protocol/ContentFrameParser.h>
#include <kord/protocol/JointFirmwareCommand.h>
#include <kord/protocol/KORDFrames.h>
#include <kord/protocol/RequestResponses/Response.h>
#include <kord/protocol/StatusFrameParser.h>
#include <kord/system/SystemAlarm.h>
#include <kord/system/SystemEvent.h>
#include <kord/system/CircularBuffer.hpp>
#include <kord/utils/utils.h>

namespace kr2::kord {

/**
 * @typedef DefaultContentItem
 * @brief Shortcut for a ContentItem with size = @ref kr2::kord::protocol::MAX_UDP_DATA_LEN
 */
using DefaultContentItem = protocol::ContentItem<protocol::MAX_UDP_DATA_LEN>;

/**
 * @typedef TCPContentItem
 * @brief Shortcut for a ContentItem with size = @ref kr2::kord::protocol::MAX_TCP_DATA_LEN
 */
using TCPContentItem = protocol::ContentItem<protocol::MAX_TCP_DATA_LEN>;

/**
 * @enum TrackingType
 * @brief Type indicates the units used in the tracking value. It serves as a hint of what
 * will drive the movement.
 */
enum TrackingType {
    TT_NONE,            //!< Unused.
    TT_TIME,            //!< The tracking value is in seconds. The movement should take the defined time.
    TT_WS_TARGET_SPEED, //!< The linear tracking value is in meters per second. The TCP should achieve the defined speed.
    TT_JS_TARGET_SPEED, //!< The tracking value is in radians per second. The joints should achieve the defined speed.
    TT_SP_APX_SPEED,    //!< The spline tracking value is in meters per second. The TCP should achieve the defined speed.
    TT_SP_CNST_SPEED,   //!< The spline tracking value is in meters per second. The TCP should achieve the defined speed.
    TT_SP_DURATION      //!< The spline tracking value is in seconds. The movement should take the defined time.
};

/**
 * @enum BlendType
 * @brief Type indicates the blending method used to blend the movement.
 */
enum BlendType {
    BT_NONE,            //!< Unused.
    BT_TIME,            //!< The blending value is in seconds. The blending should start at the defined time.
    BT_WS_ACCELERATION, //!< The blending value is in meters per second squared. Zone is defined by the radius in which
                        //!< the TCP reaches cruise section of the movement.
    BT_WS_RADIUS, //!< The blending value is in meters. Zone is defined by the radius in which the TCP reaches cruise
                  //!< section of the movement.
    BT_JS_ACCELERATION //!< The blending value is in radians per second squared. Zone is defined by the radius in which
                       //!< the joint reaches cruise section of the movement.
};

/**
 * @enum OverlayType
 * @brief Type indicates how the ending of the movement should be handled.
 */
enum OverlayType {
    OT_NONE,     //!< Unused.
    OT_VIAPOINT, //!< After entering the blend zone, movement is adjusted to the next target point, which may not be
                 //!< reached at all.
    OT_STOPPOINT //!< The TCP comes to a strand still at the exact location provided by the waypoint.
};

/**
 * @struct LoadData
 * @brief Structure that holds load-related parameters including pose, mass, center of gravity, and inertia.
 */
struct LoadData {
    std::array<double, 6ul> pose;    //!< Pose: [x, y, z, r, p, y] or similar reference.
    double mass;                     //!< Mass of the load in kilograms.
    std::array<double, 3ul> cog;     //!< Center of gravity coordinates.
    std::array<double, 6ul> inertia; //!< Inertia tensor values.
};

/**
 * @struct FrameData
 * @brief Structure that holds frame-related data, identified by a map from an ID to its pose array.
 */
struct FrameData {
    std::map<unsigned int, std::array<double, 6>> pose_; //!< For tfc and wf references
};

/**
 * @struct CBunReceivedStatistics
 * @brief Structure holding statistics for CBun data reception, including counters and jitter metrics.
 */
struct CBunReceivedStatistics {
    uint32_t fail_to_read_empty; //!< Counter of how many times recvfrom returned 0 read bytes.
    uint32_t fail_to_read_error; //!< Counter of how many times recvfrom returned < 0 read bytes.

    int64_t cmd_jitter_window_max;    //!< Command jitter window maximum.
    int64_t cmd_jitter_window_avg;    //!< Command jitter window average.
    int64_t cmd_jitter_global_max;    //!< Command jitter global maximum.
    int64_t round_trip_window_max;    //!< Round trip window maximum.
    int64_t round_trip_window_avg;    //!< Round trip window average.
    int64_t round_trip_global_max;    //!< Round trip global maximum.
    int64_t cmd_lost_window_seq;      //!< Command lost window sequence.
    int64_t cmd_lost_global_seq;      //!< Command lost global sequence.
    int64_t cmd_lost_window_timestmp; //!< Command lost window timestamp.
    int64_t cmd_lost_global_timestmp; //!< Command lost global timestamp.
    int32_t system_jitter_window_max; //!< System jitter window maximum.
    int32_t system_jitter_window_avg; //!< System jitter window average.
    int32_t system_jitter_global_max; //!< System jitter global maximum.
};

/**
 * @typedef token_t
 * @brief Token type used for request/command tracking.
 */
typedef int64_t token_t;

/**
 * @enum EFrameID
 * @brief Selector of the System Frame.
 */
enum EFrameID {
    ROBOT_BASE_FRAME = 0,       //!< Robot Base Frame
    TFC_FRAME = 7,              //!< TFC - Tool Flange Center
    TCP_FRAME = 8,              //!< TCP - Tool Center Point
    LAST_TARGET_POSE_FRAME = 9, //!< Last Target Pose
    TFC_FRAME_QUAT = 100,       //!< TFC Frame (Quaternion)
    TCP_FRAME_QUAT = 101,       //!< TCP Frame (Quaternion)
};

/**
 * @enum EFrameValue
 * @brief Selector for Frame values.
 */
enum EFrameValue {
    POSE_VAL_REF_WF = 0, //!< Rotation and Position coordinates ([x, y, z, r, p, y]) in World Frame Reference
    POSE_VAL_REF_TFC = 1 //!< Rotation and Position coordinates ([x, y, z, r, p, y]) in TFC Reference
};

/**
 * @enum ELoadID
 * @brief Selector of System Load identifiers.
 */
enum ELoadID {
    LOAD1 = 0, //!< LOAD1 - End of Arm Tool
    LOAD2 = 1  //!< LOAD2 - Payload
};

/**
 * @enum ELoadValue
 * @brief Selector which Load values to retrieve.
 */
enum ELoadValue {
    POSE_VAL = 0,   //!< Rotation and Position coordinates ([x, y, z, r, p, y]) - reference chosen on the tablet
    MASS_VAL = 2,   //!< Mass
    COG_VAL = 3,    //!< Center of Gravity ([x, y, z])
    INERTIA_VAL = 4 //!< Inertia
};

/// @brief No synchronization flags set.
enum { F_NONE = 0 };

/// @brief Using full rotation should be used only for a non-realtime communication or at init of communication.
enum { F_SYNC_FULL_ROTATION = 1 };

/**
 * @enum EAPIStatistics
 * @brief Selector which API statistics to retrieve.
 */
enum EAPIStatistics { // clang-format off
    MAX_RX_GLOBAL,                    //!< Global Max API Rx difference
    MIN_RX_GLOBAL,                    //!< Global Min API Rx difference
    AVG_RX_GLOBAL,                    //!< Average API Rx difference
    RSHB_JITTER_MAX_LOCAL,            //!< Max API Rx jitter (on the set window)
    RSHB_JITTER_MIN_LOCAL,            //!< Min API Rx jitter (on the set window)
    RSHB_JITTER_AVG_LOCAL,            //!< Average API Rx jitter (on the set window)
    RSHB_JITTER_MAX_GLOBAL,           //!< Max API Rx jitter (global)
    RSHB_JITTER_AVG_GLOBAL,           //!< Avg API Rx jitter (global)
    MAX_TX_GLOBAL,                    //!< Max CBun Tx difference
    MIN_TX_GLOBAL,                    //!< Min CBun Tx difference
    AVG_TX_GLOBAL,                    //!< Average CBun Tx difference
    MAX_LOST_API,                     //!< Consecutive packets loss max number (on the set window) for API
    MAX_LOST_CBUN,                    //!< Consecutive packets loss max number (on the set window) for CBun
    MAX_LOST_SEQ,                     //!< Consecutive packets loss max number (on the set window) for SEQ
    RSHB_CONS_LOST_COUNTER_MAX_LOCAL, //!< Consecutive packets loss max number (local) - alias for MAX_LOST_SEQ
    RSHB_CONS_LOST_COUNTER_MAX_GLOBAL,//!< Consecutive packets loss max number (global)
    MIN_LOST_API,                     //!< Consecutive packets loss min number (on the set window) for API
    MIN_LOST_CBUN,                    //!< Consecutive packets loss min number (on the set window) for CBun
    MIN_LOST_SEQ,                     //!< Consecutive packets loss min number (on the set window) for SEQ
    AVG_LOST_API,                     //!< Consecutive packets loss average number (on the set window) for API
    AVG_LOST_CBUN,                    //!< Consecutive packets loss average number (on the set window) for CBun
    AVG_LOST_SEQ,                     //!< Consecutive packets loss average number (on the set window) for SEQ
    RSHB_CONS_LOST_COUNTER_AVG_LOCAL, //!< Consecutive packets loss average number (local) - alias for AVG_LOST_SEQ
    LOST_TOTAL_API,                   //!< Total number of packets lost (on the set window) for API
    LOST_TOTAL_CBUN,                  //!< Total number of packets lost (on the set window) for CBun
    LOST_TOTAL_SEQ,                   //!< Total number of packets lost (on the set window) for SEQ
    RSHB_LOST_COUNTER_LOCAL,          //!< Lost counter local - alias for LOST_TOTAL_SEQ
    FAILED_RCV                        //!< Number of failures to receive packets (by timeout)
}; // clang-format on

/**
 * @class KordCore
 * @brief KordCore class takes care of communication management with KORD CBun, including
 * sending commands, reading statuses, and synchronizing the real-time system.
 *
 */
class KordCore {
public:
    /**
     * @struct CommandStatus
     * @brief Structure that holds a token and an error code for a command status.
     */
    struct CommandStatus {
        uint64_t token{};    //!< The token for this command.
        int8_t error_code{}; //!< The error code for this command.
    };

    /**
     * @struct CommandStatuses
     * @brief CommandStatuses structure for handling received CommandStatus
     */
    struct CommandStatuses {
        CircularBuffer<CommandStatus> statuses_{MAX_COMMAND_STATUSES}; //!< Circular buffer of CommandStatuses.

        /**
         * @brief Add a CommandStatus to the list of statuses.
         * @param a_status The CommandStatus to be added.
         */
        void addStatus(const CommandStatus &a_status)
        {
            latest_status_token_id_ = a_status.token;
            if (!isTokenPresent(a_status.token)) {
                this->statuses_.push_back(a_status);
            }
        }

        /**
         * @brief Check if the provided token matches some present CommandStatus, and assign if so
         * @param a_token The CommandStatus token to look for.
         * @param a_status The found CommandStatus structure if present.
         * @return True if the status was found, false otherwise.
         */
        bool getPresentStatus(const uint64_t &a_token, CommandStatus &a_status) const
        {
            if (auto found = std::find_if(statuses_.begin(),
                                          statuses_.end(),
                                          [a_token](CommandStatus i) { return i.token == a_token; });
                found != std::end(statuses_)) {
                a_status = *found;
                return true;
            }
            return false;
        }

        /**
         * @brief Get the latest status token ID.
         * @return The latest status token ID.
         */
        uint64_t getLatestStatusToken() const { return latest_status_token_id_; }

        /**
         * @brief Reset the list of Statuses
         */
        void reset() { this->statuses_.clear(); }

        /**
         * @brief Check if the Statuses are empty
         * @return true if the list is empty, false otherwise
         */
        [[nodiscard]] bool is_empty() const { return this->statuses_.empty(); }

    private:
        /**
         * @brief Check if a particular token is present in CommandStatuses
         * @param a_token The token to look for.
         * @return True if found, false otherwise.
         */
        bool isTokenPresent(const uint64_t &a_token)
        {
            return std::find_if(statuses_.begin(), statuses_.end(), [a_token](CommandStatus i) {
                       return i.token == a_token;
                   }) != std::end(statuses_);
        }

        static const unsigned int MAX_COMMAND_STATUSES{100}; //!< Maximum number of CommandStatuses.
        uint64_t latest_status_token_id_{}; //!< Latest added status token ID.
    };

    /**
     * @struct RobotArmStatus
     * @brief Holds status variables related to the robot arm, including hardware, software,
     *        channel quality, IO data, loads, frames, CPU states, etc.
     */
    struct RobotArmStatus { // clang-format off
        double iob_temperature_;                    //!< Measured IOBoard temperature (C°).
        std::array<double, 7UL> positions_;         //!< Model joint positions.
        std::array<double, 7ul> speed_;            //!< Model joint speeds.
        std::array<double, 7ul> accelerations_;    //!< Model joint accelerations.
        std::array<double, 7ul> torques_;          //!< Model joint torques.
        std::array<double, 7ul> sens_torques_;     //!< Sensed model joint torques.
        std::array<double, 7ul> torque_deviation_smooth_; //!< Model joint torque deviation (smoothed).

        std::array<double, 7ul> jbs_board_temperatures_;  //!< Sensor data, board temperature measurements (C°) for all 7 joints.
        std::unordered_map<unsigned int, LoadData> loads_; //!< Loads attached to the system.
        std::unordered_map<unsigned int, FrameData> frames_; //!< Frames data.
        std::unordered_map<unsigned int, double> cpu_state_; //!< CPU state data.

        std::array<double, 7ul> jbs_joint_encoder_temperatures_;  //!< Sensor data, joint encoder temperature measurements (C°) for all 7 joints.
        std::array<double, 7ul> jbs_rotor_encoder_temperatures_;  //!< Sensor data, rotor encoder temperature measurements (C°) for all 7 joints.

        std::array<double, 7ul> error_bits_;  //!< Latest obtained error bits from each joint.
        std::array<double, 7ul> status_bits_; //!< Latest obtained status bits from each joint.

        std::array<double, 6ul> tcp_model_;            //!< Current TCP of the robot (model-based).
        std::array<double, 7ul> tcp_model_quaternion_; //!< Current TCP of the robot (model-based), quaternion representation.

        std::array<std::array<double, 7ul>, 7ul> joint_quaternion_; //!< Per-joint quaternion, dimension 7x7.

        std::array<double, 6ul> tcp_sensor_; //!< Current TCP of the robot (sensor-based).

        CBunReceivedStatistics cbun_stats_; //!< CBun statistics parsed from the StatusFrame.

        int64_t min_tick_delay;      //!< Minimum tick delay over the reception time period.
        int64_t max_tick_delay;      //!< Maximum tick delay over the reception time period.
        int64_t average_tick_delay;  //!< Average tick delay over the reception time period.

        int64_t min_delay;           //!< Minimum frame reception delay over the reception period.
        int64_t max_delay;           //!< Maximum frame reception delay over the reception period.
        int64_t average_delay;       //!< Average frame reception delay over the reception period.

        int64_t faulty_frames_start; //!< Number of faulty frames from the start.
        size_t max_frames_in_tick;   //!< Maximum number of frames which were present in one tick.

        unsigned int rc_hw_flags_;           //!< Current HW status flags of the responsive controller.
        unsigned int rc_safety_flags_;       //!< Current safety flags of the responsive controller.
        unsigned int rc_motion_flags_;       //!< Current motion flags of the responsive controller.
        unsigned int rc_button_flags_;       //!< Current Button status flags of the responsive controller.
        unsigned int rc_system_alarm_state_; //!< Current system alarm state.
        unsigned int rc_safety_mode_;        //!< Safety mode set by the user.
        int64_t rc_digital_output_;          //!< Current digital outputs.
        int64_t rc_digital_input_;           //!< Current digital inputs.
        uint32_t rc_safe_digital_config;     //!< Safe digital outputs configuration.

        double rc_master_speed_; //!< Master speed set by the user in range 0 to 1.0.

        RequestSystem latest_request_system_; //!< Echo of the last captured request to the KORD CBun.

        std::array<kr2::kord::protocol::SystemEvent, 10ul> system_events_; //!< Holds latest system events.

        std::array<double, 5ul> cpu_state_temperatures_; //!< CPU state temperatures.
    }; // clang-format on

    /**
     * @struct RobotArmCommand
     * @brief Robot arm command holds control references for movements in joint space,
     *        direct control, firmware commands, or torque control.
     */
    struct RobotArmCommand {

        /**
         * @enum EType
         * @brief Defines the type of command being sent to the robot arm.
         */
        enum EType {
            eInvalid = 0,  //!< Default invalid command.
            eMOVE,         //!< Control in joint-space by providing the target joint configurations.
            eDJC,          //!< Direct Joint Control references requiring joint pos, acceleration, and speed.
            eMOVEManifold, //!< Self-motion.
            eFW,           //!< Firmware commands (e.g. brake release/engage/clear).
            eDTC           //!< Direct torque control references requiring joint torque.
        };

        EType command_type = eInvalid;            //!< Command type as defined above.
        unsigned int seq_{};                      //!< Sequence number of the robot arm commands.
        std::array<double, 7UL> positions_{};     //!< Reference joint positions.
        std::array<double, 7UL> speed_{};         //!< Reference joint speed.
        std::array<double, 7UL> accelerations_{}; //!< Reference joint acceleration.
        std::array<double, 7UL> torque_{};        //!< Reference joint torque.
        TrackingType tt_{};                       //!< Reference tracking type.
        double tt_value_{};                       //!< Reference tracking value.
        BlendType bt_{};                          //!< Reference blend type.
        double bt_value_{};                       //!< Reference blend value.
        OverlayType ot_{};                        //!< Reference overlay type.
        double sync_time_{};                      //!< Reference sync time.
        double manifold_joint_speed_{};           //!< Self-motion joint speed.

        std::array<kr2::kord::protocol::EJointFirmwareCommand, 7UL> fw_cmds_{}; //!< Firmware commands.
    };

    /**
     * @struct RobotSetupCommand
     * @brief RobotSetup command holds references for modifying robot parameters such as frames,
     * loads, or cleaning alarms.
     */
    struct RobotSetupCommand {
        /**
         * @enum EType
         * @brief Defines the type of setup command.
         */
        enum EType {
            SET_FRAME,  //!< Set frame parameters.
            SET_LOAD,   //!< Set load parameters.
            CLEAN_ALARM //!< Clean alarm flags.
        };

        unsigned int command_type; //!< One of the above EType values: \b SET_FRAME, \b SET_LOAD, \b CLEAN_ALARM
        unsigned int seq_;         //!< Sequence number of the command.

        // Frame
        std::array<double, 6UL> pose_; //!< Pose array for frame commands.
        unsigned int frame_id_;        //!< Frame ID.
        unsigned int ref_frame_;       //!< Reference frame ID.

        // Load
        double load_mass_;                     //!< Load mass for load commands.
        unsigned int load_id_;                 //!< Load ID.
        std::array<double, 3UL> load_cog_;     //!< Load center of gravity for load commands.
        std::array<double, 6UL> load_inertia_; //!< Load inertia for load commands.

        // Alarms
        unsigned int alarm_id_; //!< Alarm ID for cleaning alarms.
    };

    /**
     * @struct StatusFrameEcho
     * @brief Status Frame Echo structure for echoing timestamp data back to the robot.
     */
    struct StatusFrameEcho {
        unsigned int tx_time_stamp_; //!< Time Stamp Tx (from CBun)
        unsigned int rx_time_stamp_; //!< Time Stamp Rx (from API)
    };

    /**
     * @struct RobotFrameCommand
     * @brief Frame command holds references for the linear movement in different modes
     * (pose, velocity, dynamic, or with quaternion).
     */
    struct RobotFrameCommand {
        /**
         * @brief Defines the type of frame command.
         */
        enum {
            MOVE_POSE,      //!< Move by providing the positional references.
            MOVE_POSE_QUAT, //!< Move by providing the positional references with rotation provided as quaternion.
            MOVE_VELOCITY,  //!< Move by providing the velocity references.
            MOVE_POSE_DYN   //!< Move by providing pose, velocity, and acceleration references.
        };

        unsigned int command_type;                    //!< One of the above command types.
        unsigned int seq_;                            //!< Sequence number of the command.
        std::array<double, 6UL> tcp_target_;          //!< Reference position of the TCP.
        std::array<double, 7UL> tcp_quat_target_;     //!< Reference position of the TCP with quaternion representation.
        std::array<double, 6UL> tcp_target_velocity_; //!< Reference velocity of the TCP.
        std::array<double, 6UL> tcp_target_acc_;      //!< Reference acceleration of the TCP.
        TrackingType tt_;                             //!< Reference to tracking type.
        double tt_value_;                             //!< Reference to tracking value.
        BlendType bt_;                                //!< Reference to blend type.
        double bt_value_;                             //!< Reference to blend value.
        OverlayType ot_;                              //!< Reference to overlay type.
        double sync_time_;                            //!< Reference to synchronization time.
        long sync_value_;                             //!< Reference to synchronization value.
        double period_;                               //!< Reference to period.
        double timeout_;                              //!< Reference to timeout.

        /**
         * @brief Use the command in position control mode - only TCP pose references.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &asPoseControl();

        /**
         * @brief Use the command in position control mode - only TCP pose references with quaternion.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &asPoseQuatControl();

        /**
         * @brief Use command in the velocity control mode. The velocity of the TCP will be used as a reference.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &asVelocityControl();

        /**
         * @brief Pose, velocity and acceleration are all used as a reference for the TCP.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &asPosVelControl();

        /**
         * @brief Pass the TCP pose reference.
         * @param arr The array representing TCP pose.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withTargetPose(const std::array<double, 6UL> &arr);

        /**
         * @brief Pass the TCP pose reference with rotation provided as quaternion.
         * @param arr The array representing TCP quaternion pose.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withTargetPoseQuat(const std::array<double, 7UL> &arr);

        /**
         * @brief Pass the TCP velocity reference.
         * @param arr The array representing TCP velocity.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withTargetVelocity(const std::array<double, 6UL> &arr);

        /**
         * @brief Pass the TCP acceleration reference.
         * @param arr The array representing TCP acceleration.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withTargetAcceleration(const std::array<double, 6UL> &arr);

        /**
         * @brief Set the sequence number for this command.
         * @param seq The sequence number.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &setSequenceNumber(unsigned int seq);

        /**
         * @brief Set the tracking type.
         * @param ttype The TrackingType to set.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withTrackingType(const TrackingType &ttype);

        /**
         * @brief Set the tracking value.
         * @param val The tracking value.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withTrackingValue(const double &val);

        /**
         * @brief Set the blend type.
         * @param btype The BlendType to set.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withBlendType(const BlendType &btype);

        /**
         * @brief Set the blend value.
         * @param val The blend value.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withBlendValue(const double &val);

        /**
         * @brief Set the overlay type.
         * @param otype The OverlayType to set.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withOverlayType(const OverlayType &otype);

        /**
         * @brief Set the synchronization time.
         * @param time The sync time value.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withSyncTime(const double &time);

        /**
         * @brief Set the synchronization value.
         * @param val The sync value.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withSyncValue(const long &val);

        /**
         * @brief Set the period of the command.
         * @param p The period value.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withPeriod(const double &p);

        /**
         * @brief Set the timeout of the command.
         * @param t The timeout value.
         * @return Reference to this RobotFrameCommand object for chaining.
         */
        RobotFrameCommand &withTimeout(const double &t);
    };

    /**
     * @brief Create a new KordCore object.
     * @param hostname IP address of host where the CBun runs.
     * @param port Port where the CBun is listening. Default is 7582.
     * @param session_id A session ID for this connection.
     * @param conn Connection type. Currently only UDP type is supported.
     *
     * KordCore is communication interface between the KORD-API and the
     * KORD CBun. It is based on sockets and allows creation of a UDP server
     * and client based on the \p conn param.
     */
    explicit KordCore(const std::string &hostname, unsigned int port, unsigned int session_id, connection conn);

    /**
     * @brief Calls \p disconnect first and then destroys the Kord Core object.
     */
    ~KordCore();

    /**
     * @brief Initializes the internal tick timer and creates the connection interface.
     * @param device The device to connect to (optional).
     * @return True if the connection is established successfully, false otherwise.
     */
    bool connect(const char *device = nullptr);

    /**
     * @brief Terminates the connection interface. On success the underlying socket is closed.
     * @return true If the socket is closed successfully.
     * @return false If the socket is not closed successfully.
     */
    bool disconnect();

    /**
     * @brief Transmits a request to the remote host to start dissipation of
     * the status frame repeatedly. The remote host response will be
     * synchronized with the RC tick start on the remote host. By default, the sync
     * completes after all rotating items in the status frame are transferred. To
     * complete the rotation can take up to 50 ms based on items rotated.
     *
     * This command should be used only once to initiate the communication. Any
     * following sync should be done by waitSync.
     *
     * @param flags Flags to pass parameters to do a specific sync, first bit set for full rotation
     * @return true If Heartbeat is captured.
     * @return false If Heartbeat is not captured.
     */
    bool syncRC(long flags = F_SYNC_FULL_ROTATION);

    /**
     * @brief Blocks until the heart beat frame is captured or the timeout has elapsed.
     * It stores captured data to the appropriate structure (currently to a status
     * frame). If the capture was successful, a receiving time stamp is recorded.
     * Moreover, the capture statistics are updated with the timestamp.
     * In case of a failure, the failure to read count is increased.
     *
     * @param timeout_s How long to wait for heartbeat capture before timing out, in microseconds.
     * @param flags Flags to pass parameters to do a specific sync, first bit set for full rotation.
     * @return true If heartbeat is successfully captured.
     * @return false If heartbeat is not captured within the timeout.
     */
    bool waitSync(std::chrono::microseconds timeout_s = std::chrono::microseconds(8000), long flags = F_NONE);

    /**
     * @brief Sleep until the next time slice. There are 4ms updates.
     * After the function entry, the current tick is determined based on the
     * heartbeat reception timestamp. Synchronize with RC first by calling syncRC.
     */
    void spin();

    /**
     * @brief Returns the latest update tick timestamp.
     * @return A struct timespec with the latest update tick time.
     */
    [[nodiscard]] struct timespec ctlrcUpdateTS() const;

    /**
     * @brief Forced set of the update tick.
     * @param new_ts The new timespec to set for the update tick.
     */
    void setCtlrcUpdateTS(struct timespec new_ts);

    /**
     * @brief Send Arm Status request.
     * @return Number of bytes transmitted.
     */
    unsigned int requestArmStatus();

    /**
     * @brief Updates the provided RobotArmStatus object with the latest status from the KordCore.
     * @param arm_status RobotArmStatus object to be updated.
     */
    void updateRecentArmStatus(RobotArmStatus &arm_status) const;

    /**
     * @brief Returns the status of the latest request captured by the remote controller into the provided object.
     * @param arm_status A RobotArmStatus object to hold the current status.
     */
    void getRecentArmStatus(RobotArmStatus &arm_status) const;

    /**
     * @brief Updates the provided CommandStatuses object with the latest command status from the KordCore.
     * @param a_command_statuses CommandStatuses object to be updated.
     */
    void updateRecentCommandStatus(CommandStatuses &a_command_statuses) const;

    /**
     * @brief Returns the status of the latest command from robot status captured by the remote controller.
     * @param a_command_statuses CommandStatuses object to hold the current command statuses.
     */
    void getRecentCommandStatus(CommandStatuses &a_command_statuses) const;

    /**
     * @brief Fills the arm command and sends it to the robot arm.
     * @param cmd A RobotArmCommand struct containing the command parameters.
     * @param[out] out_token A token that will be set to track this command (optional).
     * @return Number of bytes transmitted.
     */
    unsigned int sendCommand(RobotArmCommand cmd, int64_t &out_token);

    /**
     * @brief Fills the arm command and sends it to the robot arm. Overload without token.
     * @param cmd A RobotArmCommand struct containing the command parameters.
     * @return Number of bytes transmitted.
     */
    unsigned int sendCommand(RobotArmCommand cmd);

    /**
     * @brief Fills the frame command and sends it to the robot arm.
     * @param cmd A RobotFrameCommand struct containing the command parameters.
     * @return Number of bytes transmitted.
     */
    unsigned int sendCommand(RobotFrameCommand cmd);

    /**
     * @brief Fills the setup command and sends it to the robot arm.
     * @param cmd A RobotSetupCommand struct containing the command parameters.
     * @param[out] out_token A token that will be set to track this command (optional).
     * @return Number of bytes transmitted.
     */
    unsigned int sendCommand(RobotSetupCommand cmd, int64_t &out_token);

    /**
     * @brief Fills the request and sends it to the robot arm.
     * @param req A Request object containing the request parameters.
     * @return Number of bytes transmitted.
     */
    unsigned int sendCommand(const Request &req);

    /**
     * @brief Fills the request and sends it to the robot arm, capturing a token.
     * @param req A Request object containing the request parameters.
     * @param[out] out_token A token that will be set to track this command.
     * @return Number of bytes transmitted.
     */
    unsigned int sendCommand(const Request &req, token_t &out_token);

    /**
     * @brief Fills the echo structure and sends it to the robot arm.
     * @param echo A StatusFrameEcho structure containing the echo data.
     * @return Number of bytes transmitted.
     */
    unsigned int sendCommand(const StatusFrameEcho &echo);

    /**
     * @brief Print statistics of capturing the heartbeat frame.
     * @param stats A CBunReceivedStatistics struct containing the statistics to print.
     */
    void printStats(const CBunReceivedStatistics &stats);

    /**
     * @brief Set window length in elements for the jitter capturing.
     * @param length The new window length.
     */
    void setStatisticsWindow(int length);

    /**
     * @brief Get API statistics
     * @param stat The EAPIStatistics enum entry to query.
     * @return The corresponding statistic value (in nanoseconds if time-related).
     */
    int64_t getAPIStatistics(EAPIStatistics stat);

    /**
     * @brief Get the current status frame transmit timestamp.
     * @return The transmit timestamp as int64_t.
     */
    [[nodiscard]] int64_t getTxStamp() const;

    /**
     * @brief Get the current iterative frame ID from the status.
     * @return A uint8_t representing the iterative frame ID.
     */
    [[nodiscard]] uint8_t getIterativeFrameId() const;

    /**
     * @brief Get the current iterative load ID from the status.
     * @return A uint8_t representing the iterative load ID.
     */
    [[nodiscard]] uint8_t getIterativeLoadId() const;

    /**
     * @brief Get the current iterative CPU state ID from the status.
     * @return A uint8_t representing the iterative CPU state ID.
     */
    [[nodiscard]] uint8_t getIterativeCPUStateId() const;

    /**
     * @brief Gets the recent request responses.
     * @return A vector of Response objects containing the latest responses.
     */
    [[nodiscard]] std::vector<protocol::Response> getResponses();

    /**
     * @brief Deletes the response from the list of responses with the matching token.
     * @param token The token identifying the response to erase.
     * @return True if the response was erased, false otherwise.
     */
    bool eraseResponse(token_t token);

private:
    /**
     * @brief Take the joint space references and translate them to the content item.
     * @param item Pointer to the content item to be filled.
     * @param in_jcmd The RobotArmCommand struct containing the command references.
     */
    void makeCommandMoveJ(DefaultContentItem *item, const KordCore::RobotArmCommand &in_jcmd);

    /**
     * @brief Take linear references and store them to the content item.
     * @param item Pointer to the content item to be filled.
     * @param in_lcmd The RobotFrameCommand struct containing the command references.
     */
    void makeCommandMoveL(DefaultContentItem *item, const KordCore::RobotFrameCommand &in_lcmd);

    /**
     * @brief Take linear references with rotation provided as quaternion and store them to the content item.
     * @param item Pointer to the content item to be filled.
     * @param in_lcmd The RobotFrameCommand struct containing the command references.
     */
    void makeCommandMoveLQuat(DefaultContentItem *item, const KordCore::RobotFrameCommand &in_lcmd);

    /**
     * @brief Take joint speed and create a content item for self-motion.
     * @param item Pointer to the content item to be filled.
     * @param a_cmd The RobotArmCommand struct containing the command references.
     */
    void makeCommandMoveManifold(DefaultContentItem *item, const KordCore::RobotArmCommand &a_cmd);

    /**
     * @brief Take input references and create the content item for velocity control.
     * @param item Pointer to the content item to be filled.
     * @param in_lcmd The array of velocity references in [x, y, z, rx, ry, rz].
     * @param seq_num The sequence number to place in the content item.
     * @param sync The sync value.
     * @param period The period for velocity control.
     * @param timeout The timeout for velocity control.
     */
    void makeCommandMoveVelocityL(DefaultContentItem *item,
                                  const std::array<double, 6UL> &in_lcmd,
                                  unsigned int seq_num,
                                  long sync,
                                  double period,
                                  double timeout);
    /**
     * @brief Take input references and create the content item for control by position,
     * velocity and acceleration.
     * @param item Pointer to the content item to be filled.
     * @param in_lcmd The array of position references [x, y, z, rx, ry, rz].
     * @param seq_num The sequence number to place in the content item.
     */
    void makeCommandMoveDynL(DefaultContentItem *item, const std::array<double, 6UL> &in_lcmd, unsigned int seq_num);

    /**
     * @brief Take input references and create the content item for control by joint position,
     * joint velocity, joint acceleration and torque.
     * @param item Pointer to the content item to be filled.
     * @param a_j_dcmd Joint position references.
     * @param a_jd_dcmd Joint velocity references.
     * @param a_jdd_dcmd Joint acceleration references.
     * @param a_torque_dcmd Joint torque references.
     * @param a_seq_num Sequence number to place in the content item.
     */
    void makeCommandMoveD(DefaultContentItem *item,
                          const std::array<double, 7UL> &a_j_dcmd,
                          const std::array<double, 7UL> &a_jd_dcmd,
                          const std::array<double, 7UL> &a_jdd_dcmd,
                          const std::array<double, 7UL> &a_torque_dcmd,
                          unsigned int a_seq_num);

    /**
     * @brief Take input references and create the content item for control by torque.
     * @param a_content_item Pointer to the content item to be filled.
     * @param a_torque_dcmd Joint torque references.
     * @param a_seq_num Sequence number to place in the content item.
     */
    void makeCommandMoveDirectTorque(DefaultContentItem *a_content_item,
                                     const std::array<double, 7UL> &a_torque_dcmd,
                                     unsigned int a_seq_num);

    /**
     * @brief Make the initial arm status request to initiate the dissipation of the heartbeat.
     * @param item Pointer to the content item to be filled with the request.
     */
    void makeArmStatusRequest(DefaultContentItem *item);

    /**
     * @brief Convert the joint firmware request to the appropriate content item.
     * @param item Pointer to the content item to be filled.
     * @param in_fw_cmd Firmware commands array.
     * @param seq_num Sequence number to place in the content item.
     * @param a_timeStamp Output parameter to store the generated timestamp for this command.
     */
    void makeCommandFirmware(DefaultContentItem *item,
                             const std::array<kr2::kord::protocol::EJointFirmwareCommand, 7UL> &in_fw_cmd,
                             unsigned int seq_num,
                             int64_t &a_timeStamp);

    /**
     * @brief Take input reference and create the content item for set Frame Command.
     * @param a_content_item Pointer to the content item to be filled.
     * @param a_lcmd The RobotSetupCommand struct containing the frame command parameters.
     * @param a_timeStamp Output parameter to store the generated timestamp for this command.
     */
    void makeCommandSetFrame(DefaultContentItem *a_content_item,
                             const KordCore::RobotSetupCommand &a_lcmd,
                             int64_t &a_timeStamp);

    /**
     * @brief Take input reference and create the content item for set Load Command.
     * @param a_content_item Pointer to the content item to be filled.
     * @param a_lcmd The RobotSetupCommand struct containing the load command parameters.
     * @param a_timeStamp Output parameter to store the generated timestamp for this command.
     */
    void makeCommandSetLoad(DefaultContentItem *a_content_item,
                            const KordCore::RobotSetupCommand &a_lcmd,
                            int64_t &a_timeStamp);

    /**
     * @brief Take input reference and create the content item for set Clean Alarm Command.
     * @param a_content_item Pointer to the content item to be filled.
     * @param a_lcmd The RobotSetupCommand struct containing the clean alarm command parameters.
     * @param a_timeStamp Output parameter to store the generated timestamp for this command.
     */
    void makeCommandCleanAlarm(DefaultContentItem *a_content_item,
                               const KordCore::RobotSetupCommand &a_lcmd,
                               int64_t &a_timeStamp);

    /**
     * @brief Take a request and fill in the content frame with a request content item.
     * @param item Pointer to the content item to be filled.
     * @param req The Request object to process.
     * @param seq Sequence number to assign.
     * @return Integer status code (0 on success).
     */
    int setRequestContentItem(DefaultContentItem *item, const Request &req, unsigned short seq);

    /**
     * @brief Take a request and fill in the content frame with a request content item (TCP version).
     * @param item Pointer to the content item to be filled.
     * @param req The Request object to process.
     * @param seq Sequence number to assign.
     * @return Integer status code (0 on success).
     */
    int setRequestContentItem(TCPContentItem *item, const Request &req, unsigned short seq);

    /**
     * @brief Take an echo and fill in the content frame with an echo content item.
     * @param a_content Pointer to the content item to be filled.
     * @param a_echo The StatusFrameEcho object containing the timestamp data.
     * @return Integer status code (0 on success).
     */
    int setStatusEchoContentItem(DefaultContentItem *a_content, const KordCore::StatusFrameEcho &a_echo);

    /**
     * @brief Dispatch the frame to the remote controller.
     * @param frame A pointer to the KORDFrame object to be sent.
     * @return Number of bytes transmitted.
     */
    unsigned int sendFrame(const kr2::kord::protocol::KORDFrame *frame);

    /**
     * @brief Waits for a valid frame within the timeout period.
     * @param a_timeout_us The timeout duration in microseconds.
     * @param time_out_counter Reference to a counter tracking the number of timeout attempts.
     * @return True if a valid frame is received within the timeout, false otherwise.
     */
    bool waitForValidFrame(std::chrono::microseconds a_timeout_us, int &time_out_counter);

    /**
     * @brief Checks if the elapsed time has exceeded the specified timeout.
     * @param start The start time point.
     * @param timeout The timeout duration.
     * @return True if the elapsed time has exceeded the timeout, false otherwise.
     */
    static bool hasTimedOut(const std::chrono::steady_clock::time_point &start,
                            const std::chrono::microseconds &timeout);

    /**
     * @brief Receives a TCP frame if TCP connection is used.
     */
    void receiveTCPFrame();

    /**
     * @brief Parses the frame from the received payload.
     */
    void parseFrame();

    /**
     * @brief Updates the frame, load, and CPU state IDs, handling the first and second loops.
     * @param got_first_id Reference to a flag indicating if the first ID has been obtained.
     * @param second_loop Reference to a flag indicating if the second loop has started.
     * @param first_frame_id Reference to the first frame ID.
     * @param now_frame_id Reference to the current frame ID.
     * @param first_load_id Reference to the first load ID.
     * @param now_load_id Reference to the current load ID.
     * @param first_cpu_state_id Reference to the first CPU state ID.
     * @param now_cpu_state_id Reference to the current CPU state ID.
     * @return True if IDs are updated, false otherwise.
     */
    bool updateIds(bool &got_first_id,
                   bool &second_loop,
                   int &first_frame_id,
                   int &now_frame_id,
                   int &first_load_id,
                   int &now_load_id,
                   int &first_cpu_state_id,
                   int &now_cpu_state_id) const;

    /**
     * @brief Updates recent statuses for arm and command.
     */
    void updateRecentStatuses();

    /**
     * @brief Checks if the data is within the recent interval.
     * @param recent_interval The interval considered as recent.
     * @return True if the data is recent, false otherwise.
     */
    [[nodiscard]] bool isRecent(const std::chrono::nanoseconds &recent_interval) const;

    /**
     * @brief Determines if the main loop should continue based on the conditions.
     * @param not_recent Flag indicating if the data is not recent.
     * @param full_cycle Flag indicating if a full cycle rotation is required.
     * @param second_loop Flag indicating if the second loop has started.
     * @param first_frame_id The first frame ID.
     * @param now_frame_id The current frame ID.
     * @param first_load_id The first load ID.
     * @param now_load_id The current load ID.
     * @param first_cpu_state_id The first CPU state ID.
     * @param now_cpu_state_id The current CPU state ID.
     * @return True if the loop should continue, false otherwise.
     */
    static bool shouldContinueLoop(bool not_recent,
                                   bool full_cycle,
                                   bool second_loop,
                                   int first_frame_id,
                                   int now_frame_id,
                                   int first_load_id,
                                   int now_load_id,
                                   int first_cpu_state_id,
                                   int now_cpu_state_id);

    /**
     * @brief Determines the correct connection container (TCP or UDP) for the given request
     *        and prepares the corresponding frame.
     * @param a_request The request to process.
     * @return A shared pointer to the ConnectionInterface object.
     */
    std::shared_ptr<ConnectionInterface> determineContainerAndPrepareFrame(const Request &a_request);

    /**
     * @brief Checks if the given request should be sent over TCP.
     * @param a_request The request to evaluate.
     * @return True if the request is TCP-based, otherwise false.
     */
    [[nodiscard]] bool isTCPRequest(const Request &a_request) const;

    /**
     * @brief Prepares the TCP frame for the specified request.
     * @param a_request The request for which the TCP frame is to be prepared.
     */
    void prepareTCPFrame(const Request &a_request);

    /**
     * @brief Prepares the UDP frame for the specified request.
     * @param a_request The request for which the UDP frame is to be prepared.
     */
    void prepareUDPFrame(const Request &a_request);

    /**
     * @brief Sends the pre-prepared frame using the specified connection container.
     * @param container A shared pointer to the ConnectionInterface to use for sending.
     * @param a_request The request associated with the frame.
     * @return Number of bytes transmitted.
     */
    unsigned int sendPreparedFrame(std::shared_ptr<ConnectionInterface> &container, const Request &a_request);

    std::shared_ptr<ConnectionInterface> conn_container_;          //!< Connection container for the main interface.
    std::shared_ptr<ConnectionInterface> conn_container_requests_; //!< Connection container for requests (TCP if used).

    connection conn_type_; //!< The connection type (UDP or TCP, etc.).

    asio::io_service io_service_;          //!< IO service for main communication.
    asio::io_service io_requests_service_; //!< IO service for request communication.

    std::string hostname_;    //!< Hostname or IP address for the remote KORD.
    unsigned int port_;       //!< Port for the remote KORD.
    unsigned int session_id_; //!< Session ID for this connection.

    /**
     * @class Internals
     * @brief A forward-declared class to hold internal data and implementation details.
     */
    class Internals;
    Internals *his_; //!< Pointer to internal implementation details.

    unsigned int packets_cnt_ = 0;                             //!< Count of packets sent/received.
    std::chrono::time_point<std::chrono::steady_clock> start_; //!< Start time for statistics.

    utils::StampsLog ts_record_; //!< Timestamp log structure for synchronization.
    uint16_t sequence_number_{}; //!< Sequence number management.

    timespec ctlrc_update_ts_{0, 0}; //!< Last update timespec from the robot.

    utils::Stats stats; //!< Utility stats for API usage.
};

} // namespace kr2::kord

#endif // KR2_KORD_CORE_H
