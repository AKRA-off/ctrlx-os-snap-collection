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

#ifndef KR2_KORD_SYSTEM_EVENTS_H
#define KR2_KORD_SYSTEM_EVENTS_H

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <kord/system/CircularBuffer.hpp>

namespace kr2::kord::protocol {

/**
 * @brief EEvent group determines the origin of the content retrieved.
 */
enum EEventGroup {
    eUnknown = 0,   //!<@brief The ids were not member of any group. This is for debugging and internal use.
    eSafetyEvent,   //!<@brief Safety event group is a group of events related to external devices, which
                    //! are for example in EStop state or their measurements are inconsistent
                    //! with references, which most of the time results in stopping of the robot.
    eSoftStopEvent, //!<@brief SoftStop event group originates from the controller and is related to the
                    //! robot model and its limits and to controller internal integrity.
                    //! The robot is stopped when the limits are exceeded.
    eKordEvent      //!<@brief Kord event group is related to errors within the KORD infrastructure. Currently
                    //! it is used for communication errors between the API and CBun, eventually when the CBUN
                    //! fails to pass commands to the controller.
};

/**
 * @brief ID of the SafetyEvent condition that took place. This helps to determine the cause of the failure.
 */
enum ESafetyEventConditionID {
    EXTERNAL_ESTOP_ACTIVATED = 1001,    //!< @brief EStop was activated by either the EStop button or by a device.
    IOB_NOT_RESPONDING = 1002,          //!< @brief Communication with IOB was interrupted for more than 5 updates
                                        //! in a row. The result will be emergency stop and the power will be removed
                                        //! from the robot arm. Initialization will be needed.
    JBS_NOT_RESPONDING = 1003,          //!< @brief Communication with one or more of JBs was interrupted
                                        //! for more than 5 updates in a row. The result will be emergency stop
                                        //! and the power will be removed from the robot arm.
    JREF_X_SENSOR_POSITION_SPAN = 1004, //!< @brief The model joint position and sensor joint position
                                        //! have deviated over a given threshold during movement.
    JREF_POSITION_DELTA_SPAN = 1005,    //!< @brief The difference between two consecutive joint
                                        //! positions in the model has exceeded acceptable limit.
    JRATED_SPEED_EXCEEDED = 1006,       //!< @brief Joint model speed has exceeded rated joint
                                        //! speed limit and appropriate safety action will be taken.
    JRATED_TORQUE_EXCEEDED = 1007,      //!< @brief Joint model torque has exceeded rated joint
                                        //! torque limit and appropriate safety action will be taken.
    JHOLD_TORQUE_EXCEEDED = 1008,       //!< @brief Calculated static joint torque has exceeded static
                                        //! joint torque limit. Static torque is torque when the robot
                                        //! is holding still in place.
    JBRATED_TEMP_EXCEEDED = 1009,       //!< @brief Joint rated temperature has been exceeded.
    JTORQUE_DEVIATION_EXCEEDED = 1010,  //!< @brief Joint expected and measured torques have deviated
                                        //! over acceptable threshold.
    MODEL_X_TRJ_REFJ_SSPAN_EXC = 1011,  //!< @brief The difference between trajectory reference joint
                                        //! position and model joint position has exceeded the given
                                        //! span limit. PStop action will be taken.
    MODEL_X_TRJ_REFW_SSPAN_EXC = 1012,  //!< @brief The difference between the norm of the trajectory
                                        //! reference TCP position and the norm of the model
                                        //! TCP position has exceeded the given span limit.
                                        //! Translations and rotations are considered separately.
    FRAME_SPEED_LIMIT_EXC = 1013,       //!< @brief Translation speed limit is exceeded by at least
                                        //! one of the robot frames.
    EXTERNAL_PSTOP_ACTIVATED = 2001     //!< @brief PStop was activated by either the PStop button
                                        //! or a device.
};

/**
 * @brief ID of the SoftStopEvent condition that took place in the robot.
 */
enum ESoftStopEventConditionID {
    MODEL_INVALID_STATE = 2001,        //!<@brief The robot model holds invalid values for joints or frames
                                       //! of the robot. Invalid values considered are either not a number
                                       //! or infinities.
    MODEL_JVELOCITY_LIMITS_EXC = 2002, //!<@brief Robot model joints velocity has exceeded limits.
    MODEL_JTORQUE_LIMITS_EXC = 2003,   //!<@brief The model torque has exceeded the current torque limits.
                                       //! The torque limits for this case take into account the joint speed
                                       //! and adjust the limit for the current speed.
    MODEL_JSDTORQUE_LIMITS_EXC = 2004, //!<@brief The generated torque has changed between two updates
                                       //! and the new torque value is exceeding the static torque limits.
                                       //! This event is reported for example when load is changed.
    MODEL_JPOS_LIMITS_VIOLATION_EST =
        2005,                  //!<@brief Robot model joints have violated the maximum or minimum positions of joints.
    INIT_JOINTS_LOCKED = 2006, //!<@brief The initialization failed because at least one of the joints
                               //! could not be unlocked.
};

/**
 * @brief ID of the KordEvent condition that took place in the robot.
 */
enum EKordEventConditionID {
    CBUN_KORD_BAD_CONN_QUALITY = 3001, //!<@brief The connection quality between the API and CBun exceeds the
                                       //! configured limits.
    INFEASIBLE_MOVE_COMMAND =
        3002, //!<@brief The CBun has detected that the movement is not feasible
              //! and it will be stopped. The motion constraints can be configured via configuration file.
    CBUN_KORD_COMM_ERROR [[maybe_unused]] = 3003, //!<@brief [Unused] The communication between the API and CBun
                                                  //! has been interrupted for more than 3 updates in a row.
};

/**
 * @brief SystemEvent structure for transferring robot system events.
 */
struct SystemEvent {
    int64_t timestamp_{}; //!<@brief Timestamp of the event.

    uint32_t event_id_{}; //!<@brief Event ID. It is used to determine the cause of the failure.
                          //!< Represented by \p ESafetyEventConditionID, \p ESoftStopEventConditionID
                          //!< and \p EKordEventConditionID.

    uint16_t event_group_{}; //!<@brief Event group means the origin of the event.
                             //!< Represented by \p EEventGroup struct. \par
                             //!< Possible values: \p eSafetyEvent, \p eSoftStopEvent and \p eKordEvent.

    /**
     * @brief Resets the \p SystemEvent.
     */
    void reset()
    {
        this->timestamp_ = 0;
        this->event_id_ = 0;
        this->event_group_ = 0;
    }

    bool operator==(const SystemEvent &a_rhs) const
    {
        return this->timestamp_ == a_rhs.timestamp_ && this->event_group_ == a_rhs.event_group_ &&
               this->event_id_ == a_rhs.event_id_;
    }

    /**
     * @brief Converts the \p SystemEvent to byte array.
     *
     * @param bytes_output Output vector of bytes.
     */
    void toByteArray(std::vector<uint8_t> &bytes_output) const
    {
        bytes_output.push_back(timestamp_ >> 56);
        bytes_output.push_back(timestamp_ >> 48);
        bytes_output.push_back(timestamp_ >> 40);
        bytes_output.push_back(timestamp_ >> 32);
        bytes_output.push_back(timestamp_ >> 24);
        bytes_output.push_back(timestamp_ >> 16);
        bytes_output.push_back(timestamp_ >> 8);
        bytes_output.push_back(timestamp_);

        bytes_output.push_back(event_id_ >> 24);
        bytes_output.push_back(event_id_ >> 16);
        bytes_output.push_back(event_id_ >> 8);
        bytes_output.push_back(event_id_);

        bytes_output.push_back(event_group_ >> 8);
        bytes_output.push_back(event_group_);
    }

    /**
     * @brief Convert byte array to \p SystemEvent.
     *
     * @param a_in_bytes Input vector of bytes.
     */
    bool initFromByteArray(std::vector<uint8_t> &a_in_bytes)
    {
        if (a_in_bytes.size() < sizeof(SystemEvent)) {
            return false;
        }

        union {
            int64_t l;
            uint8_t s[8];
        } ts{};

        ts.s[7] = a_in_bytes[0];
        ts.s[6] = a_in_bytes[1];
        ts.s[5] = a_in_bytes[2];
        ts.s[4] = a_in_bytes[3];
        ts.s[3] = a_in_bytes[4];
        ts.s[2] = a_in_bytes[5];
        ts.s[1] = a_in_bytes[6];
        ts.s[0] = a_in_bytes[7];

        timestamp_ = ts.l;

        union {
            uint32_t u;
            uint8_t s[4];
        } eid{};

        eid.s[3] = a_in_bytes[8];
        eid.s[2] = a_in_bytes[9];
        eid.s[1] = a_in_bytes[10];
        eid.s[0] = a_in_bytes[11];
        event_id_ = eid.u;

        event_group_ = uint16_t(a_in_bytes[12]) << 8;
        event_group_ |= a_in_bytes[13];

        return true;
    }

    [[nodiscard]] std::string _toString(const std::string &t, double time_multiplier = 1e-9) const
    {
        std::string tm = get_converted_value(time_multiplier, 0);

        std::string ret = " [Timestamp|" + tm + "]:" + t + " [ID]:" + std::to_string(this->event_id_) +
                          " [Event Group]:" + std::to_string(this->event_group_);
        return ret;
    }

    /**
     * @brief Get string representation of \p SystemEvent.
     *
     * @param time_multiplier A constant for time conversion. Default: nanoseconds.
     * @return String representation: timestamp_, event_id_ and event_group_.
     */
    [[nodiscard]] std::string toString(double time_multiplier = 1.0) const
    {
        int64_t dt = this->timestamp_; // nanosec
        std::string t = get_converted_value(dt * time_multiplier, 2);
        return _toString(t, time_multiplier);
    }

    /**
     * @brief Get string representation of \p SystemEvent with milliseconds representation.
     *
     * @param a_event \p SystemEvent to be compared with.
     * @param time_multiplier A constant for time conversion. Default: nanoseconds.
     * @return String representation: timestamp_, event_id_ and event_group_.
     */
    [[nodiscard]] std::string toStringWithRef(const SystemEvent &a_event, double time_multiplier = 1e-6) const
    {
        int64_t dt = this->timestamp_ - a_event.timestamp_; // nanosec
        std::string t = "+" + get_converted_value(dt * time_multiplier, 2);
        return _toString(t, time_multiplier);
    }

    static std::string get_converted_value(double v, int precision = 2)
    {
        std::stringstream _stream;

        if (precision == 0)
            _stream << std::fixed << std::setprecision(precision) << std::scientific << v;
        else
            _stream << std::fixed << std::setprecision(precision) << v;

        return _stream.str();
    }

    /**
     * @brief Check the validity of the \p SystemEvent.
     *
     * @return True if the \p SystemEvent is valid, otherwise false.
     */
    [[nodiscard]] bool is_valid() const
    {
        bool timestamp_ok = this->timestamp_ > 0;
        bool event_group_ok = true;
        return timestamp_ok and event_group_ok;
    }
} __attribute__((packed));

/**
 * @brief SystemEvents structure for storing the list of SystemEvent.
 */
struct SystemEvents {
    CircularBuffer<SystemEvent> events_{MAX_SYSTEM_EVENTS};

    /**
     * @brief Adds the SystemEvent to the list of events.
     *
     * @param a_event SystemEvent to be added.
     */
    void addEvent(const SystemEvent &a_event) { this->events_.push_back(a_event); }

    /**
     * @brief Check if the provided event is present in the list.
     *
     * @param a_event \p SystemEvent to be checked for presence.
     */
    [[nodiscard]] bool isEventPresent(const SystemEvent &a_event) const
    {
        bool found = (std::find(events_.begin(), events_.end(), a_event) != events_.end());
        return found;
    }

    /**
     * @brief Reset the list of events.
     *
     */
    void reset() { this->events_.clear(); }

    /**
     * @brief Check if there are any events in the list.
     *
     * @return True if the list is empty, otherwise false.
     *
     */
    [[nodiscard]] bool is_empty() const { return this->events_.empty(); }

private:
    // Limit the size to 7, more would overflow the Status frame
    // Once the request response frames are incorporated,
    // it will be possible to stack the error messages and tx multiple
    // without restriction on size. So far the events are stored in the
    // status frame, where the size is rather limited.
    static const unsigned int MAX_SYSTEM_EVENTS{7};
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_SYSTEM_EVENTS_H
