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

#ifndef KR2_KORD_RECEIVER_INTERFACE_H
#define KR2_KORD_RECEIVER_INTERFACE_H

#include <kord/api/kord.h>
#include <kord/utils/worker.h>

#include <bitset>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace kr2::kord {

/**
 * @class ReceiverInterface
 * @brief Interface for capturing data from the remote controller.
 *
 * The ReceiverInterface provides functionalities for monitoring and evaluating the state of the system.
 * It captures data such as joint values, frame values, CPU states, system alarms, and various statistics
 * related to the transmission channel.
 */
class ReceiverInterface {
public:
    mutable std::mutex comm_mutex_; /**< Mutex for thread-safe operations */

    /**
     * @brief Constructs a new ReceiverInterface object.
     *
     * @param kord Shared pointer to an instance of the \b KordCore class, providing the communication interface.
     */
    explicit ReceiverInterface(std::shared_ptr<KordCore> kord);

    /**
     * @brief Destroys the ReceiverInterface object.
     */
    ~ReceiverInterface();

    /**
     * @brief Synchronizes by waiting for the capture of the heartbeat from the remote controller.
     *
     * This function updates the internal capture state based on the heartbeat signal.
     *
     * @return true if synchronized successfully.
     * @return false if synchronization failed or timed out.
     */
    bool sync();

    /**
     * @brief Copies data from the capture buffer to the state structures.
     *
     * This allows the data to be retrieved from the state structures.
     */
    void fetchData();

    /**
     * @brief Fetches the current status from the remote controller and updates internal state.
     *
     * This function waits for a status update from the remote controller and then calls
     * `fetchData()` to update the internal state structures. It ensures that the ReceiverInterface
     * has the latest system information.
     *
     * @return true if the status was successfully fetched and updated.
     * @return false if the operation timed out or failed.
     */
    bool fetchStatus();

    /**
     * @brief Sends a status echo to the remote controller.
     *
     * @param timestamp The timestamp associated with the status echo.
     * @return true if the echo was sent successfully.
     * @return false otherwise.
     */
    bool sendStatusEcho(int64_t timestamp);

    // Joint values

    /**
     * @enum EJointValue
     * @brief Enumeration for selecting which joint values to retrieve.
     */
    enum class EJointValue {
        T_REFERENCE_Q = 1,        /**< Target reference joint configuration in radians. */
        T_REFERENCE_QD = 2,       /**< Reference joint speed in rad/sec. */
        T_REFERENCE_QDD = 3,      /**< Reference joint accelerations in rad/sec². */
        T_REFERENCE_TRQ = 4,      /**< Reference torque in Nm. */
        S_ACTUAL_Q = 5,            /**< Actual joint configurations from sensors. */
        S_ACTUAL_QD = 6,           /**< Actual joint speeds from sensors. */
        S_ACTUAL_QDD = 7,          /**< Actual joint accelerations from sensors. */
        S_ACTUAL_TRQ = 8,          /**< Torque estimation. */
        S_TEMP_BOARD = 9,          /**< Joint Board temperature sensors. */
        S_TEMP_JOINT_ENCODER = 10, /**< Joint Encoder temperature sensors. */
        S_TEMP_ROTOR_ENCODER = 11, /**< Rotor Encoder temperature sensors. */
        S_SENSED_TRQ = 12,         /**< Joint Sensed Torques. */
        S_TRQDEV_SMOOTH = 13       /**< Joint Torque Deviation (Smoothed). */
        // S_TRQDEV_RAW = 14,      /**< Joint Torque Deviation (Raw). Uncomment if needed. */
    };

    /**
     * @brief Retrieves the specified joint values.
     *
     * This method is thread-safe and returns the requested joint values based on the provided selector.
     *
     * @param value The \b EJointValue specifying which joint values to retrieve.
     * @return std::array<double, 7UL> An array holding the specified joint values.
     */
    std::array<double, 7UL> getJoint(EJointValue value);

    /**
     * @brief Retrieves frame values based on the specified frame ID and value selector.
     *
     * This method is thread-safe and returns the requested frame values.
     *
     * @param e_frame_id Identifier for the frame of reference.
     * @param e_frame_value Selector specifying the particular value to retrieve within the frame.
     * @return std::vector<std::variant<double, int>> A vector holding the specified frame values, typically in the form [x (m), y (m), z (m), r (rads), p (rads), y (rads)].
     */
    std::vector<std::variant<double, int>> getFrame(EFrameID e_frame_id, EFrameValue e_frame_value);

    /**
     * @brief Retrieves the status of a command using a token.
     *
     * Refer to `ECommandStatusFlags` for possible return values. This method is thread-safe.
     *
     * @param token The token associated with the command.
     * @return int8_t The status flags represented by an 8-bit integer.
     */
    int8_t getCommandStatus(token_t token);

    /**
     * @brief Retrieves load values based on the specified load ID and value selector.
     *
     * This method is thread-safe and returns the requested load values.
     *
     * @param e_load_id Identifier for the type of load.
     * @param e_load_value Selector specifying the particular load value to retrieve.
     * @return std::vector<std::variant<double, int>> A vector holding the specified load values. For Center of Gravity
     * (CoG): [x, y, z] in meters; for Mass: [m] in kilograms; for Inertia: [xx, yy, zz, xy, xz, yz] in kg·m².
     */
    std::vector<std::variant<double, int>> getLoad(ELoadID e_load_id, ELoadValue e_load_value);

    /**
     * @brief Retrieves the actual TCP coordinates in the World Frame reference.
     *
     * This method is thread-safe and returns the TCP position and orientation.
     *
     * @return std::array<double, 6UL> An array holding TCP coordinates in the form [x (m), y (m), z (m), r (rads), p (rads), y (rads)].
     */
    std::array<double, 6UL> getTCP() const;

    /**
     * @brief Retrieves the actual TCP coordinates with quaternion representation in the World Frame reference.
     *
     * This method is thread-safe and returns the TCP position and orientation as a quaternion.
     *
     * The returned array contains the TCP's position and orientation as follows:
     * [x (m), y (m), z (m), qx, qy, qz, qw]
     *
     * @return std::array<double, 7UL> An array holding TCP coordinates in the form [x (m), y (m), z (m), qx, qy, qz, qw].
     */
    std::array<double, 7UL> getTCPWithQuaternion() const;

    /**
     * @brief Retrieves the actual joint pose with quaternion representation.
     *
     * This method is thread-safe and returns the pose of the specified joint.
     *
     * @param joint_num The joint number (allowed values are 1-7).
     * @return std::array<double, 7UL> An array holding the joint pose in the form [x (m), y (m), z (m), qx, qy, qz, qw].
     */
    std::array<double, 7UL> getJointPoseWithQuaternion(int joint_num) const;

    // State information

    /**
     * @brief Retrieves the hardware status flags.
     *
     * This method is thread-safe and returns the hardware status flags.
     *
     * @return unsigned int The hardware flags, see \link kr2::kord::protocol::EHWFlags \endlink.
     */
    unsigned int getHWFlags() const;

    /**
     * @brief Retrieves the button status flags.
     *
     * This method is thread-safe and returns the button status flags.
     *
     * @return unsigned int The button flags, see \link kr2::kord::protocol::EButtonFlags \endlink.
     */
    unsigned int getButtonFlags() const;

    /**
     * @enum ECPUStateValue
     * @brief Enumeration for selecting which CPU state values to retrieve.
     */
    enum ECPUStateValue {
        PACKAGE_ID0_TEMP = 0, /**< Package ID 0 temperature (average). */
        CORE_0_TEMP = 1,      /**< Core 0 temperature. */
        CORE_1_TEMP = 2,      /**< Core 1 temperature. */
        CORE_2_TEMP = 3,      /**< Core 2 temperature. */
        CORE_3_TEMP = 4,      /**< Core 3 temperature. */
    };

    /**
     * @brief Retrieves CPU state values based on the specified selector.
     *
     * For CPU temperatures, if **lm-sensors version 1:3.4.0-4** is not installed on the controller,
     * all returned values are 0.
     *
     * @param cpu_state_id The \b ECPUStateValue specifying which CPU state to retrieve.
     * @return double The value of the specified CPU state.
     */
    double getCPUState(unsigned int cpu_state_id);

    /**
     * @brief Retrieves the latest command token.
     *
     * This method is thread-safe and returns the latest command token.
     *
     * @return uint64_t The latest command token.
     */
    uint64_t getLatestCommandToken() const;

    /**
     * @brief Retrieves the robot safety flags.
     *
     * @return unsigned int The safety flags.
     */
    unsigned int getRobotSafetyFlags() const;

    /**
     * @brief Retrieves the master speed from the system.
     *
     * @return double The master speed ranging from 0 to 1.0.
     */
    double getMasterSpeed() const;

    /**
     * @brief Retrieves the IOBoard temperature from the system.
     *
     * @return double The IOBoard temperature.
     */
    double getIOBoardTemperature() const;

    /**
     * @brief Retrieves the safety mode of the robot.
     *
     * @return int An integer representing the safety mode.
     */
    int getSafetyMode() const;

    /**
     * @brief Retrieves the motion flags from the robot.
     *
     * @return unsigned int The motion flags.
     */
    unsigned int getMotionFlags() const;

    /**
     * @brief Retrieves the encoded alarm state.
     *
     * In the absence of any present error or alarm state, the function returns 0.
     *
     * The alarms and errors are encoded as follows:
     *
     * **SAFETY EVENT (read from CAT):**
     *
     * ```
     * 31             23     20         15            7       3     0
     *  +---------------------+-----------------------+-------+-----+
     *  |      reserved       |   Condition ID        | CNTXT | CAT |
     *  +---------------------+-----------------------+-------------+
     * ```
     *
     * **CAT (Category):**
     * - 1 = SafetyEvent
     * - 2 = SoftStopEvent
     * - 3 = HwStat
     * - 4 = CBunEvent
     *
     * **CNTXT (Context):**
     * - 0x01 = ESTOP
     * - 0x02 = PSTOP
     * - 0x04 = SSTOP
     * - 0x08 = SYSERR
     *
     * For more details regarding the *conditions coding*, please refer to the
     * [Appendix](../api/appendix.html).
     *
     * @return uint32_t The encoded error state or 0 if no alarms/errors are present.
     */
    uint32_t systemAlarmState() const;

    // Statistics values

    /**
     * @enum EStatsValue
     * @brief Enumeration for selecting which statistics values to retrieve.
     */
    enum class EStatsValue {
        FAIL_TO_READ_EMPTY = 0,       /**< Number of times recvfrom returned 0. */
        FAIL_TO_READ_ERROR = 1,       /**< Number of times recvfrom returned <0. */
        CMD_JITTER_MAX_LOCAL = 2,     /**< Max deviation from expected synchronicity of commands (local, in micros). */
        CMD_JITTER_AVG_LOCAL = 3,     /**< Avg deviation from expected synchronicity of commands (local, in micros). */
        CMD_JITTER_MAX_GLOBAL = 4,    /**< Max deviation from expected synchronicity of commands (global, in micros). */
        ROUND_TRIP_TIME_MAX_LOCAL = 5,/**< Max time elapsed between tick start and capture of command (local, in micros). */
        ROUND_TRIP_TIME_AVG_LOCAL = 6,/**< Avg time elapsed between tick start and capture of command (local, in micros). */
        ROUND_TRIP_TIME_MAX_GLOBAL = 7,/**< Max time elapsed between tick start and capture of command (global, in micros). */
        CMD_LOST_COUNTER_LOCAL = 8,    /**< Number of lost commands (local). */
        CMD_LOST_COUNTER_GLOBAL = 9,   /**< Number of lost commands (global). */
        CMD_LOST_COUNTER_LOCAL_TIMESTAMP = 10, /**< Timestamp for local lost commands. */
        CMD_LOST_COUNTER_GLOBAL_TIMESTAMP = 11, /**< Timestamp for global lost commands. */
        SYS_JITTER_MAX_LOCAL = 12,     /**< Max time deviation of control thread wake-up time (local, in micros). */
        SYS_JITTER_AVG_LOCAL = 13,     /**< Avg time deviation of control thread wake-up time (local, in micros). */
        SYS_JITTER_MAX_GLOBAL = 14,    /**< Max time deviation of control thread wake-up time (global, in micros). */
    };

    /**
     * @brief Retrieves a specific statistics value.
     *
     * @param stats_value The \b EStatsValue specifying which statistics value to retrieve.
     * @return int64_t The requested statistics value.
     */
    int64_t getStatistics(EStatsValue stats_value);

    /**
     * @brief Retrieves the structure containing all parsed statistics.
     *
     * @return CBunReceivedStatistics The structure holding parsed statistics data.
     */
    CBunReceivedStatistics getStatisticsStructure() const;

    /**
     * @brief Retrieves the latest digital IO output status.
     *
     * @return int64_t Bit representation of the digital output status.
     */
    int64_t getDigitalOutput() const;

    /**
     * @brief Retrieves the latest digital IO input status.
     *
     * @return int64_t Bit representation of the digital input status.
     */
    int64_t getDigitalInput();

    /**
     * @brief Retrieves the latest safe digital IO configuration.
     *
     * Each 8 bits represent the configuration value of the i-th safe output.
     *
     * @return uint32_t The safe digital output configuration.
     */
    uint32_t getSafeDigitalOutputConfig() const;

    /**
     * @brief Formats the input bits into a human-readable string.
     *
     * Example format: "0000 0000 ... 0000".
     *
     * @return std::string The formatted input bits.
     */
    std::string getFormattedInputBits();

    /**
     * @brief Formats the output bits into a human-readable string.
     *
     * @return std::string The formatted output bits.
     */
    std::string getFormattedOutputBits() const;

    /**
     * @brief Reports the maximum number of frames that were captured in a single time slot.
     *
     * @return size_t The maximum number of frames present in one tick.
     */
    size_t getMaxFramesInTick() const;

    /**
     * @brief Reports the number of times the receiver was unable to read data from the socket.
     *
     * @return int64_t The count of faulty tick frames.
     */
    int64_t getFaultyTickFrame() const;

    /**
     * @brief Retrieves the latest request data.
     *
     * @return Request The latest request sent to the remote controller.
     */
    Request getLatestRequest();

    /**
     * @brief Checks if the response to a specific request is available.
     *
     * @param token The token associated with the request.
     * @return true If the response is available.
     * @return false Otherwise.
     */
    bool hasResponse(token_t token) const;

    /**
     * @brief Retrieves the response object for a specific request token.
     *
     * @tparam T The expected type of the response.
     * @param token The token associated with the request.
     * @return T The response object corresponding to the token.
     * @throws std::runtime_error If the response for the given token is not available.
     */
    template <typename T>
    T getResponse(token_t token);

    /**
     * @brief Retrieves the latest system events.
     *
     * Only event timestamp, group, and ID are reported. The system events are cleared every time
     * `waitSync` is successfully called. If the connection times out, the latest known system events
     * are returned.
     *
     * @return std::vector<kr2::kord::protocol::SystemEvent> A vector of the latest system events.
     */
    std::vector<protocol::SystemEvent> getSystemEvents();

    /**
     * @brief Clears the system event buffer.
     */
    void clearSystemEventsBuffer();

private:
    struct Internals; /**< Forward declaration of the internal implementation structure. */
    std::shared_ptr<Internals> his_; /**< Shared pointer to the internal implementation. */

    /**
     * @class HeartBeatMonitor
     * @brief Singleton class that monitors the heartbeat of the remote controller.
     *
     * Inherits from the Worker class to perform background monitoring tasks.
     */
    class HeartBeatMonitor : public Worker {
    public:
        /**
         * @brief Retrieves the singleton instance of HeartBeatMonitor.
         *
         * Initializes the monitor with the provided ReceiverInterface if not already set.
         *
         * @param a_rcv_interface_ Pointer to the ReceiverInterface to monitor.
         * @return HeartBeatMonitor& Reference to the singleton instance.
         */
        static HeartBeatMonitor &getInstance(ReceiverInterface *a_rcv_interface_)
        {
            static HeartBeatMonitor instance;
            if (!instance.is_interface_set()) {
                instance.set_interface(a_rcv_interface_);
            }
            return instance;
        }

        /**
         * @brief Retrieves the current status of the worker.
         *
         * @return true If the worker is running.
         * @return false Otherwise.
         */
        [[nodiscard]] bool get_status() const { return this->status_; }

        /**
         * @brief Resets the worker's status.
         */
        void reset() { this->status_ = true; }

        /**
         * @brief Stops the worker.
         */
        void stop() { keep_running_.store(false); }

        /**
         * @brief Checks if the interface is set.
         *
         * @return true If the interface is set.
         * @return false Otherwise.
         */
        [[nodiscard]] bool is_interface_set() const { return rcv_interface_ != nullptr; }

        /**
         * @brief Sets the ReceiverInterface for monitoring.
         *
         * @param a_rcv_interface_ Pointer to the ReceiverInterface.
         */
        void set_interface(ReceiverInterface *a_rcv_interface_) { rcv_interface_ = a_rcv_interface_; }

    private:
        std::atomic<bool> keep_running_{}; /**< Atomic flag to control the running state of the worker. */
        ReceiverInterface *rcv_interface_ = nullptr; /**< Pointer to the ReceiverInterface being monitored. */
        bool status_ = true; /**< Current status of the worker. */

        /**
         * @brief Fetches statistics from the ReceiverInterface.
         */
        void fetchStats() { this->status_ = rcv_interface_->fetchStatus(); }

        /**
         * @brief The main execution loop of the worker.
         *
         * Continuously fetches statistics while the worker is running.
         */
        void run() override
        {
            keep_running_.store(true);
            while (keep_running_.load()) {
                fetchStats();
                // You might want to add a sleep or wait mechanism here to prevent a tight loop
            }
            this->finished();
        }
    };
    HeartBeatMonitor &monitor_instance = HeartBeatMonitor::getInstance(this); /**< Instance of HeartBeatMonitor. */
};

} // namespace kr2::kord

#endif // KR2_KORD_RECEIVER_INTERFACE_H
