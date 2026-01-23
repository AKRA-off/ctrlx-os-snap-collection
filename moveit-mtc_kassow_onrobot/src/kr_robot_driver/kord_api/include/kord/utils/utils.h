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

#ifndef KR2_KORD_UTILS_H
#define KR2_KORD_UTILS_H

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <functional>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <queue>

#include <kord/system/SystemAlarm.h>
#include <kord/system/SystemEvent.h>
#include <kord/utils/logger.h>

namespace kr2::utils {

/**
 * @brief Logs timestamp information.
 */
struct StampsLog {
    /**
     * @brief Constructs a new StampsLog object.
     */
    StampsLog();

    /**
     * @brief Resets the timestamp logs.
     */
    void reset();

    /**
     * @brief Retrieves the header string for the log.
     *
     * @return A string representing the header.
     */
    std::string header();

    /**
     * @brief Converts the log to a string representation.
     *
     * @return A string containing the log information.
     */
    std::string asString();

    int64_t t0_; ///< Initial timestamp.
    int64_t t1_; ///< Intermediate timestamp.
    int64_t t2_; ///< Final timestamp.
};

/**
 * @brief Collects and manages general statistics.
 */
class StatGeneral {
public:
    /**
     * @brief Constructs a new StatGeneral object.
     */
    StatGeneral();

    /**
     * @brief Resets all statistical counters.
     */
    void reset();

    /**
     * @brief Updates the statistics with a new value.
     *
     * @param a_uval The value to update the statistics with.
     */
    void update(long a_uval);

    /**
     * @brief Prints the current statistics to the provided stream.
     *
     * @param a_stream The output stream to print the statistics to.
     */
    void printStat(std::ostream &a_stream) const;

private:
    long avg_ = 0;    ///< Average value.
    long max_ = 0;    ///< Maximum value.
    long min_ = 1e6;  ///< Minimum value.
    long counter_ = 0;///< Counter for the number of updates.
};

/**
 * @brief Computes statistics based on timepoint differences.
 */
class StatsTimepointDifference {

public:
    using timespan = std::chrono::duration<int64_t, std::nano>;

    /**
     * @brief Constructs a new StatsTimepointDifference object.
     */
    StatsTimepointDifference();

    /**
     * @brief Resets all statistical data.
     */
    void reset();

    /**
     * @brief Records a failure event with the given timestamp.
     *
     * @param a_capture_ts The timestamp of the failure.
     */
    void updateFailure(const std::chrono::time_point<std::chrono::steady_clock> &a_capture_ts);

    /**
     * @brief Records a capture event with the given timestamp.
     *
     * @param a_capture_ts The timestamp of the capture.
     */
    void updateCapture(const std::chrono::time_point<std::chrono::steady_clock> &a_capture_ts);

    /**
     * @brief Prints the collected statistics to the provided stream.
     *
     * @param a_stream The output stream to print the statistics to.
     */
    void printStats(std::ostream &a_stream) const;

private:
    unsigned long captured_message_cnt_ = 0; ///< Number of captured messages.
    unsigned int failed_capture_cnt_ = 0;    ///< Number of failed captures.

    std::chrono::time_point<std::chrono::steady_clock> cap_first_;   ///< First capture timestamp.
    std::chrono::time_point<std::chrono::steady_clock> cap_latest_;  ///< Latest capture timestamp.
    timespan min_; ///< Minimum timespan observed.
    timespan max_; ///< Maximum timespan observed.
};

/**
 * @brief Represents a long option for command-line argument parsing.
 */
struct LongOption {
    struct option long_option_;      ///< The underlying option structure.
    std::string help_string_;        ///< Help description for the option.
    std::string arg_name_;           ///< Argument name for the option.

    /**
     * @brief Constructs a new LongOption object.
     *
     * @param opt The option structure.
     * @param help The help description.
     * @param arg_name The name of the argument (default: "<arg>").
     */
    LongOption(const struct option &opt, const std::string &help, const std::string &arg_name = "<arg>")
        : long_option_(opt), help_string_(help), arg_name_(arg_name)
    {
    }

    /**
     * @brief Generates a formatted help string for the option.
     *
     * @return A string containing the formatted help information.
     */
    [[nodiscard]] std::string printHelp() const
    {
        std::stringstream ss;
        ss << "    --" << std::left << std::setw(15) << long_option_.name << " ";
        if (long_option_.has_arg == required_argument) {
            ss << "<" << arg_name_ << ">" << std::left << std::setw(10);
        }
        else if (long_option_.has_arg == optional_argument) {
            ss << "[<" << arg_name_ << ">]" << std::left << std::setw(8);
        }
        else {
            ss << std::left << std::setw(15);
        }
        ss << " " << help_string_ << std::left << std::setw(0);
        return ss.str();
    }
};

/**
 * @brief Struct of Arrays of LongOptions for processing dedicated arguments.
 *
 * @tparam N The number of long options.
 */
template <size_t N> class SOALongOptions {
public:
    /**
     * @brief Default constructor.
     */
    SOALongOptions() = default;

    /**
     * @brief Constructs a SOALongOptions object with the given array of LongOptions.
     *
     * @param a_long_options An array of LongOption objects.
     */
    explicit SOALongOptions(std::array<LongOption, N> a_long_options)
    {
        for (int i = 0; i < N; i++) {
            long_options_[i] = a_long_options[i].long_option_;
            help_strings_[i] = a_long_options[i].help_string_;
            arg_strings_[i] = a_long_options[i].arg_name_;
        }
        long_options_[N] = {0, 0, 0, 0}; // terminator
    }

    /**
     * @brief Generates a help string for all long options.
     *
     * @return A string containing the formatted help information.
     */
    [[nodiscard]] std::string helpString() const
    {
        std::stringstream ss;
        ss << "[DEDICATED OPTIONS]\n";
        for (int i = 0; i < N; i++) {
            ss << formatHelp(long_options_[i], help_strings_[i], arg_strings_[i]) << "\n";
        }
        return ss.str();
    }

    /**
     * @brief Retrieves the array of long options.
     *
     * @return A pointer to the array of struct option.
     */
    [[nodiscard]] struct option *getLongOptions() { return long_options_; }

    /**
     * @brief Retrieves the number of long options.
     *
     * @return The number of long options.
     */
    [[nodiscard]] size_t getNumberOfOptions() const { return N; }

private:
    /**
     * @brief Formats a single long option into a help string.
     *
     * @param a_option The option structure.
     * @param a_help_string The help description.
     * @param a_arg_string The argument name.
     * @return A formatted help string.
     */
    [[nodiscard]] std::string formatHelp(const option &a_option,
                                         const std::string &a_help_string,
                                         const std::string &a_arg_string) const
    {
        std::stringstream ss;
        ss << "    --" << std::left << std::setw(15) << a_option.name << " ";
        if (a_option.has_arg == required_argument) {
            ss << a_arg_string << std::left << std::setw(10);
        }
        else if (a_option.has_arg == optional_argument) {
            ss << a_arg_string << std::left << std::setw(8);
        }
        else {
            ss << std::left << std::setw(15);
        }
        ss << " " << a_help_string << std::left << std::setw(0);
        return ss.str();
    }

    option long_options_[N + 1]{}; ///< Array of long options with terminator.
    std::array<std::string, N> help_strings_; ///< Array of help strings.
    std::array<std::string, N> arg_strings_;  ///< Array of argument names.
};

/**
 * @brief Decodes system alarm states into human-readable strings.
 */
class SystemAlarmStateDecoder {
public:
    /**
     * @brief Decodes the system alarm state into a descriptive string.
     *
     * @param systemAlarmState The system alarm state as a 32-bit integer.
     * @return A string describing the system alarm state.
     */
    static std::string decodeAsString(uint32_t systemAlarmState)
    {
        std::ostringstream oss;
        oss << "System Alarm state: " << systemAlarmState << ", category: ";
        switch (systemAlarmState & 0b1111) {
        case static_cast<uint32_t>(kr2::kord::protocol::ESystemAlarmCategory::CAT_SAFETY_EVENT):
            oss << "Safety Event";
            break;
        case static_cast<uint32_t>(kr2::kord::protocol::ESystemAlarmCategory::CAT_SOFT_STOP_EVENT):
            oss << "Soft Stop Event";
            break;
        case static_cast<uint32_t>(kr2::kord::protocol::ESystemAlarmCategory::CAT_CBUN_EVENT):
            oss << "CBun Event";
            break;
        case static_cast<uint32_t>(kr2::kord::protocol::ESystemAlarmCategory::CAT_HW_STAT):
            oss << "Hardware Event";
            break;
        default:
            oss << "No Events";
            break;
        }

        oss << ", context: ";
        switch (systemAlarmState & 0b111110000) {
        case static_cast<uint32_t>(kr2::kord::protocol::ESystemAlarmContext::CNTXT_ESTOP):
            oss << "EStop";
            break;
        case static_cast<uint32_t>(kr2::kord::protocol::ESystemAlarmContext::CNTXT_PSTOP):
            oss << "PStop";
            break;
        case static_cast<uint32_t>(kr2::kord::protocol::ESystemAlarmContext::CNTXT_SSTOP):
            oss << "SSTOP";
            break;
        case static_cast<uint32_t>(kr2::kord::protocol::ESystemAlarmContext::CNTXT_SYSERR):
            oss << "SYSERR";
            break;
        default:
            oss << "No context";
        }

        return oss.str();
    }
};

/**
 * @brief Converts a string to a specified type.
 *
 * @tparam T The type to convert to.
 * @param str The string to convert.
 * @return The converted value of type T.
 */
template <class T> T convertFromString(const std::string &str)
{
    T value;
    std::istringstream iss(str);
    iss >> value;
    return value;
}

/**
 * @brief Specialization of convertFromString for std::string.
 *
 * @param str The string to convert.
 * @return The same string.
 */
template <> inline std::string convertFromString<std::string>(const std::string &str) { return str; }

/**
 * @brief Retrieves an environment variable and converts it to the specified type.
 *
 * @tparam T The type to convert the environment variable to.
 * @param key The name of the environment variable.
 * @param default_value The default value to return if the environment variable is not set.
 * @return The value of the environment variable converted to type T, or the default value.
 */
template <class T> T getEnv(const std::string &key, T default_value)
{
    const char *env_value = std::getenv(key.c_str());
    if (env_value) {
        return convertFromString<T>(env_value);
    }
    return default_value;
}

/**
 * @brief Represents launch parameters and processes command-line arguments.
 */
struct LaunchParameters {
    using ExternalArgParser = std::function<void(int)>;
    static const int INVALID_INDEX = -1;

    /**
     * @brief Processes launch arguments and initializes LaunchParameters.
     *
     * @param argc The argument count.
     * @param argv The argument vector.
     * @param a_parser External parser function for additional arguments.
     * @return An initialized LaunchParameters object.
     */
    static LaunchParameters processLaunchArguments(int argc, char **argv, const ExternalArgParser &a_parser = [](int) {})
    {
        LaunchParameters parameters;

        static option long_options[] = {
            // Add only long options shared among all examples.
            // Specific options should be added and handled in the example using the ExternalArgParser
            {0, 0, 0, 0}};

        int opt;
        int option_index = 0;
        opterr = 0;
        while ((opt = getopt_long(argc, argv, "ht:r:p:c:i:n:", long_options, &option_index)) != -1) {
            switch (opt) {
            case 'h': {
                parameters.help_ = true;
                printUsage();
                a_parser(INVALID_INDEX); // It should print help
                break;
            }
            case 'r': {
                parameters.rt_prio_ = atoi(optarg);
                break;
            }
            case 't': {
                parameters.runtime_ = std::chrono::seconds(atoi(optarg));
                break;
            }
            case 'p': {
                parameters.port_ = atoi(optarg);
                if (parameters.port_ > 65535) {
                    parameters.valid_ = false;
                    KORD_LOG_ERROR("ERROR: Invalid port number: " << parameters.port_);
                    return parameters;
                }
                break;
            }
            case 'c': {
                unsigned char buf[sizeof(struct in6_addr)];
                parameters.remote_controller_ = optarg;

                if (inet_pton(AF_INET, parameters.remote_controller_.c_str(), buf) <= 0) {
                    parameters.valid_ = false;
                    KORD_LOG_ERROR("ERROR: Malformed IP address: " << parameters.remote_controller_);
                    return parameters;
                }
                break;
            }
            case 'i': {
                parameters.session_id_ = atoi(optarg);
                if (parameters.session_id_ > 255) {
                    parameters.valid_ = false;
                    KORD_LOG_ERROR("ERROR: Invalid session ID: " << parameters.session_id_);
                    return parameters;
                }
                break;
            }
            case 0:
            case '?':
            default: {
                // If anything else is here, handover to external parser
                a_parser(optind - 1);
            }
            }
        }

        parameters.init_time_ = std::chrono::steady_clock::now();
        parameters.valid_ = true;
        return parameters;
    }

    /**
     * @brief Prints the usage information to the standard output.
     *
     * @param with_time If true, includes the runtime option in the usage.
     */
    static void printUsage(bool with_time = false)
    {
        std::cout << "[OPTIONS]\n";
        std::cout << "    -h               show this help\n";
        if (with_time)
            std::cout << "    -t <runtime>     set for how long the example should run\n";
        std::cout << "    -r <prio>        execute as a realtime process with priority set to <prio>\n";
        std::cout << "    -p <port>        port number to connect to\n";
        std::cout << "    -c <IP Address>  remote controller IP address\n";
        std::cout << "    -i <Session ID>  KORD session ID | Default: 1\n";
    }

    /**
     * @brief Checks if the specified runtime has elapsed.
     *
     * @return True if the runtime has elapsed, false otherwise.
     */
    [[nodiscard]] bool runtimeElapsed() const
    {
        if (runtime_.count() == 0) {
            return false;
        }

        if ((std::chrono::steady_clock::now() - init_time_) > runtime_) {
            return true;
        }

        return false;
    }

    /**
     * @brief Determines if real-time execution is enabled.
     *
     * @return True if real-time priority is set, false otherwise.
     */
    [[nodiscard]] bool useRealtime() const { return rt_prio_ > 0; }

    /**
     * @brief Generates a string representation of the launch parameters.
     *
     * @return A string containing all the launch parameters.
     */
    [[nodiscard]] std::string printParameters() const
    {
        std::stringstream ss;
        ss << "Started with following parameters:\n";
        ss << "    Remote RC IP/port: " << remote_controller_ << ":" << port_ << "\n";
        ss << "    Session ID       : " << session_id_ << "\n";
        ss << "    Realtime Priority: " << rt_prio_ << "\n";
        ss << "    Runtime          : " << runtime_.count() << " seconds\n";
        return ss.str();
    }

    bool valid_ = false;                          ///< Indicates if the parameters are valid.
    bool help_ = false;                           ///< Indicates if help was requested.
    int rt_prio_ = -1;                            ///< Real-time priority.
    std::chrono::seconds runtime_{0};             ///< Runtime duration.
    int port_ = 7582;                             ///< Port number.
    int session_id_ = 1;                          ///< Session ID.
    std::string remote_controller_ = getEnv<std::string>("KR2_KORD_API_ROBOT_ADDRESS", "192.168.38.1"); ///< Remote controller IP.

    std::chrono::time_point<std::chrono::steady_clock> init_time_; ///< Initialization time.
};

/**
 * @brief Collects and manages various statistics.
 */
class Stats {
public:
    /**
     * @brief Constructs a new Stats object with default values.
     */
    Stats()
        : failed_capture_cnt_(0), cnt_(0), max_rx(INT64_MIN), min_rx(INT64_MAX), max_tx(INT64_MIN), min_tx(INT64_MAX),
          recents_buffer_size_(STATS_INTERVAL_ / (KORD_REF_PERIOD_ / 1000000))
    {
    }

    /**
     * @brief Resets all statistical data.
     */
    void reset()
    {
        max_rx = INT64_MIN;
        min_rx = INT64_MAX;
        max_tx = INT64_MIN;
        min_tx = INT64_MAX;
        cnt_ = 0;
        failed_capture_cnt_ = 0;
        sum_jitter = 0;
        recents_buffer_size_ = STATS_INTERVAL_ / (KORD_REF_PERIOD_ / 1000000);
        last_received_seq_number_ = -1;
        local_packets_lost_seq = 0;
        global_packets_lost_seq = 0;
        global_cons_packets_lost_max = 0;
        clear(jitters_buffer);
        clear(lost_packets_buffer_cbun);
        clear(stats_sequences_buffer);
    }

    /**
     * @brief Captures a new event with the provided timestamps and status.
     *
     * @param a_capture_ts The capture timestamp.
     * @param a_capture_ts_cbun The CBun capture timestamp (optional).
     * @param a_status_sequence_num The status sequence number (optional).
     */
    void capture(const std::chrono::time_point<std::chrono::steady_clock> a_capture_ts,
                 int64_t a_capture_ts_cbun = -1,
                 int a_status_sequence_num = -1)
    {
        if (cnt_ == 0) {
            cap_first_ = a_capture_ts;
            cap_latest_ = a_capture_ts;
            cap_first_cbun_ = a_capture_ts_cbun;
            cap_latest_cbun_ = a_capture_ts_cbun;
            ++cnt_;
            return;
        }

        // rx analysis
        std::chrono::duration<int64_t, std::nano> elapsed_rx = a_capture_ts - cap_latest_;

        long t_diff_rx = elapsed_rx.count();

        // global extremums
        if (t_diff_rx < min_rx) {
            min_rx = t_diff_rx;
        }
        if (t_diff_rx > max_rx) {
            max_rx = t_diff_rx;
        }

        window_jitter_statistics(t_diff_rx);
        window_lost_statistics(t_diff_rx, lost_packets_buffer_api, sum_lost_api);
        // window_lost_statistics(t_diff_rx);

        if (a_capture_ts_cbun != -1) {
            // tx analysis
            int64_t t_diff_tx = a_capture_ts_cbun - cap_latest_cbun_;

            // global extremes
            if (t_diff_tx < min_tx) {
                min_tx = t_diff_tx;
            }
            if (t_diff_tx > max_tx) {
                max_tx = t_diff_tx;
            }

            window_lost_statistics(t_diff_tx, lost_packets_buffer_cbun, sum_lost_cbun);
            cap_latest_cbun_ = a_capture_ts_cbun;
        }

        if (a_status_sequence_num != -1) {
            // Processing of the status's sequence number
            processNewSequenceNumber(a_status_sequence_num);
        }

        cap_latest_ = a_capture_ts;
        ++cnt_;
    }

    /**
     * @brief Processes a new sequence number from the status.
     *
     * @param a_rcv_sequence_number The received sequence number.
     */
    void processNewSequenceNumber(const uint16_t &a_rcv_sequence_number)
    {
        checkOfTheReceivedSequenceNumberGlobal(a_rcv_sequence_number);
        checkOfTheReceivedSequenceNumberLocal(a_rcv_sequence_number);

        last_received_seq_number_ = a_rcv_sequence_number;
    }

    /**
     * @brief Checks the continuity of the received global sequence number.
     *
     * @param a_rcv_sequence_number The received sequence number.
     */
    void checkOfTheReceivedSequenceNumberGlobal(const uint16_t &a_rcv_sequence_number)
    {
        if ((last_received_seq_number_ == -1) || (a_rcv_sequence_number == 0)) {
            // start, just reset and record
            return;
        }
        int diff_ = int(a_rcv_sequence_number) - int(last_received_seq_number_);
        if (diff_ > 1) {
            // lost packet error
            global_packets_lost_seq += 1;
        }
        if (diff_ - 1 > global_cons_packets_lost_max) {
            global_cons_packets_lost_max = diff_ - 1;
        }
    }

    /**
     * @brief Checks the continuity of the received local sequence number within a statistics interval.
     *
     * @param a_rcv_sequence_number The received sequence number.
     */
    void checkOfTheReceivedSequenceNumberLocal(const uint16_t &a_rcv_sequence_number)
    {
        if ((last_received_seq_number_ == -1) || (a_rcv_sequence_number == 0)) {
            // start, just record
            return;
        }
        int diff_ = int(a_rcv_sequence_number) - int(last_received_seq_number_);
        if (diff_ > 1) { // ok difference is 1
            local_packets_lost_seq += (diff_ - 1);
            stats_sequences_buffer.push((diff_ - 1));
        }
        else {
            stats_sequences_buffer.push(0);
        }
        if (stats_sequences_buffer.size() > recents_buffer_size_) { // STATS_INTERVAL_ passed
            local_packets_lost_seq -= stats_sequences_buffer.front();
            stats_sequences_buffer.pop();
        }

        last_received_seq_number_ = a_rcv_sequence_number;
    }

    /**
     * @brief Calculates the number of lost packets based on the difference.
     *
     * @param a_diff The difference in timestamps.
     * @return The number of lost packets.
     */
    [[nodiscard]] int number_of_lost_calculation(const long &a_diff) const
    {
        if (a_diff > MAX_TX_PACKETS_DIFF_) {
            return std::max(a_diff / KORD_REF_PERIOD_ - 1, 1L);
        }
        return 0;
    }

    /**
     * @brief Calculates the jitter based on the difference.
     *
     * @param a_diff The difference in timestamps.
     * @param unused An unused parameter for future extensions.
     * @return The calculated jitter.
     */
    [[nodiscard]] int jitter_calculation(const long &a_diff, const long &) const
    {
        return static_cast<int>(std::sqrt(std::abs(a_diff * a_diff - KORD_REF_PERIOD_ * KORD_REF_PERIOD_)));
    }

    /**
     * @brief Updates jitter statistics within a sliding window.
     *
     * @param a_rx_diff The received timestamp difference.
     */
    void window_jitter_statistics(const long &a_rx_diff)
    {

        int moment_jitter = jitter_calculation(a_rx_diff, KORD_REF_PERIOD_);
        jitters_buffer.push(moment_jitter);
        sum_jitter += moment_jitter;

        if (jitters_buffer.size() > recents_buffer_size_) { // STATS_INTERVAL_ passed

            int deleted_element = jitters_buffer.front();
            sum_jitter -= deleted_element;
            jitters_buffer.pop();
        }
    }

    /**
     * @brief Updates global jitter statistics.
     *
     * @param a_rx_diff The received timestamp difference.
     */
    void global_jitter_statistics(const long &a_rx_diff)
    {

        int moment_jitter = jitter_calculation(a_rx_diff, KORD_REF_PERIOD_);
        jitters_buffer.push(moment_jitter);
        sum_jitter += moment_jitter;

        if (jitters_buffer.size() > recents_buffer_size_) { // STATS_INTERVAL_ passed

            int deleted_element = jitters_buffer.front();
            sum_jitter -= deleted_element;
            jitters_buffer.pop();
        }
    }

    /**
     * @brief Updates lost packet statistics within a sliding window.
     *
     * @param a_tx_diff The transmitted timestamp difference.
     * @param lost_packets_buffer The buffer storing lost packet counts.
     * @param sum_lost The sum of lost packets.
     */
    void window_lost_statistics(const long &a_tx_diff, std::queue<int> &lost_packets_buffer, long &sum_lost) const
    {

        int64_t moment_lost = number_of_lost_calculation(a_tx_diff);
        lost_packets_buffer.push(moment_lost);
        sum_lost += moment_lost;

        if (lost_packets_buffer.size() > recents_buffer_size_) { // STATS_INTERVAL_ passed

            int deleted_element = lost_packets_buffer.front();
            sum_lost -= deleted_element;
            lost_packets_buffer.pop();
        }
    }

    /**
     * @brief Increments the failure capture count and records the failure.
     */
    void update_failure()
    {
        ++failed_capture_cnt_;

        capture(std::chrono::steady_clock::now());
    }

    // Global statistics accessors

    /**
     * @brief Calculates the average receive time.
     *
     * @return The average receive time in nanoseconds.
     */
    [[nodiscard]] int64_t get_avg_rx() const
    {
        int64_t avg_cap =
            std::chrono::duration_cast<std::chrono::duration<int64_t, std::nano>>(cap_latest_ - cap_first_).count();
        return avg_cap / static_cast<int64_t>(cnt_);
    }

    /**
     * @brief Retrieves the maximum receive time.
     *
     * @return The maximum receive time in nanoseconds.
     */
    [[nodiscard]] int64_t get_max_rx() const { return max_rx; }

    /**
     * @brief Retrieves the minimum receive time.
     *
     * @return The minimum receive time in nanoseconds.
     */
    [[nodiscard]] int64_t get_min_rx() const { return min_rx; }

    /**
     * @brief Calculates the maximum jitter in receive times.
     *
     * @return The maximum receive jitter.
     */
    [[nodiscard]] int64_t get_max_rx_jitter() { return jitter_calculation(get_max_rx(), KORD_REF_PERIOD_); }

    /**
     * @brief Calculates the average jitter in receive times.
     *
     * @return The average receive jitter.
     */
    [[nodiscard]] int64_t get_avg_rx_jitter() { return jitter_calculation(get_avg_rx(), KORD_REF_PERIOD_); }

    /**
     * @brief Calculates the average transmit time.
     *
     * @return The average transmit time in nanoseconds.
     */
    [[nodiscard]] int64_t get_avg_tx() const
    {
        int64_t avg_cap = cap_latest_cbun_ - cap_first_cbun_;
        if (cnt_ == 0) {
            return 0;
        }
        return avg_cap / static_cast<int64_t>(cnt_);
    }

    /**
     * @brief Retrieves the maximum transmit time.
     *
     * @return The maximum transmit time in nanoseconds.
     */
    [[nodiscard]] int64_t get_max_tx() const { return max_tx; }

    /**
     * @brief Retrieves the minimum transmit time.
     *
     * @return The minimum transmit time in nanoseconds.
     */
    [[nodiscard]] int64_t get_min_tx() const { return min_tx; }

    /**
     * @brief Retrieves the maximum number of consecutive lost packets globally.
     *
     * @return The maximum consecutive lost packets.
     */
    [[nodiscard]] int64_t get_max_drop_global() const { return global_cons_packets_lost_max; }

    // Local statistics accessors

    /**
     * @brief Calculates the average jitter.
     *
     * @return The average jitter.
     */
    [[nodiscard]] int64_t get_avg_jitter() const { return sum_jitter / std::max(size_t(1), jitters_buffer.size()); }

    /**
     * @brief Retrieves the minimum jitter.
     *
     * @return The minimum jitter.
     */
    [[nodiscard]] int64_t get_min_jitter() const
    {
        int64_t min_jitter = INT64_MAX;
        std::queue<int> copy_queue = jitters_buffer;
        while (!copy_queue.empty()) {
            int current = copy_queue.front();
            if (current < min_jitter) {
                min_jitter = current;
            }
            copy_queue.pop();
        }
        return min_jitter;
    }

    /**
     * @brief Retrieves the maximum jitter.
     *
     * @return The maximum jitter.
     */
    [[nodiscard]] int64_t get_max_jitter() const
    {
        int64_t max_jitter = INT64_MIN;
        std::queue<int> copy_queue = jitters_buffer;
        while (!copy_queue.empty()) {
            int current = copy_queue.front();
            if (current > max_jitter) {
                max_jitter = current;
            }
            copy_queue.pop();
        }

        return max_jitter;
    }

    /**
     * @brief Calculates the average number of lost CBun packets.
     *
     * @return The average lost CBun packets.
     */
    [[nodiscard]] int64_t get_avg_lost_cbun() const
    {
        return sum_lost_cbun / std::max(size_t(1), lost_packets_buffer_cbun.size());
    }

    /**
     * @brief Calculates the average number of lost sequence packets.
     *
     * @return The average lost sequence packets.
     */
    [[nodiscard]] int64_t get_avg_lost_seq() const
    {
        return local_packets_lost_seq / std::max(size_t(1), stats_sequences_buffer.size());
    }

    /**
     * @brief Calculates the average number of lost API packets.
     *
     * @return The average lost API packets.
     */
    [[nodiscard]] int64_t get_avg_lost_api() const
    {
        return sum_lost_api / std::max(size_t(1), lost_packets_buffer_api.size());
    }

    /**
     * @brief Retrieves the minimum number of lost CBun packets.
     *
     * @return The minimum lost CBun packets.
     */
    [[nodiscard]] int64_t get_min_lost_cbun() { return get_min_lost(lost_packets_buffer_cbun); }

    /**
     * @brief Retrieves the minimum number of lost API packets.
     *
     * @return The minimum lost API packets.
     */
    [[nodiscard]] int64_t get_min_lost_api() { return get_min_lost(lost_packets_buffer_api); }

    /**
     * @brief Retrieves the minimum number of lost sequence packets.
     *
     * @return The minimum lost sequence packets.
     */
    [[nodiscard]] int64_t get_min_lost_seq() { return get_min_lost(stats_sequences_buffer); }

    /**
     * @brief Retrieves the maximum number of lost CBun packets.
     *
     * @return The maximum lost CBun packets.
     */
    [[nodiscard]] int64_t get_max_lost_cbun() { return get_max_lost(lost_packets_buffer_cbun); }

    /**
     * @brief Retrieves the maximum number of lost API packets.
     *
     * @return The maximum lost API packets.
     */
    [[nodiscard]] int64_t get_max_lost_api() { return get_max_lost(lost_packets_buffer_api); }

    /**
     * @brief Retrieves the maximum number of lost sequence packets.
     *
     * @return The maximum lost sequence packets.
     */
    [[nodiscard]] int64_t get_max_lost_seq() { return get_max_lost(stats_sequences_buffer); }

    /**
     * @brief Retrieves the total number of failed captures.
     *
     * @return The count of failed captures.
     */
    [[nodiscard]] int64_t get_failed_cnt() const { return failed_capture_cnt_; }

    /**
     * @brief Sets the size of the statistics window.
     *
     * @param a_number_of_recents The number of recent entries to consider.
     */
    void set_stats_window(const int32_t &a_number_of_recents) { recents_buffer_size_ = a_number_of_recents; }

    /**
     * @brief Clears the contents of a queue.
     *
     * @param q The queue to clear.
     */
    static void clear(std::queue<int> &q)
    {
        std::queue<int> empty;
        std::swap(q, empty);
    }

private:
    /**
     * @brief Retrieves the minimum lost packets from a buffer.
     *
     * @param buffer The queue containing lost packet counts.
     * @return The minimum number of lost packets.
     */
    static int64_t get_min_lost(std::queue<int> &buffer)
    {
        int64_t min_lost = INT64_MAX;
        // finding minimum using timestamps
        std::queue<int> copy_queue = buffer;
        while (!copy_queue.empty()) {
            int current = copy_queue.front();
            if (current < min_lost) {
                min_lost = current;
            }
            copy_queue.pop();
        }
        return min_lost;
    }

    /**
     * @brief Retrieves the maximum lost packets from a buffer.
     *
     * @param buffer The queue containing lost packet counts.
     * @return The maximum number of lost packets.
     */
    static int64_t get_max_lost(std::queue<int> &buffer)
    {
        int64_t max_lost = INT64_MIN;
        // finding maximum using timestamps
        std::queue<int> copy_queue = buffer;
        while (!copy_queue.empty()) {
            int current = copy_queue.front();
            if (current > max_lost) {
                max_lost = current;
            }
            copy_queue.pop();
        }
        return max_lost;
    }

public:
    std::chrono::time_point<std::chrono::steady_clock> cap_first_;    ///< First capture timestamp.
    std::chrono::time_point<std::chrono::steady_clock> cap_latest_;   ///< Latest capture timestamp.

    int64_t cap_first_cbun_{};     ///< First CBun capture timestamp.
    int64_t cap_latest_cbun_{};    ///< Latest CBun capture timestamp.

    int64_t failed_capture_cnt_;   ///< Count of failed captures.

    size_t cnt_;                   ///< Total number of captures.

    int64_t max_rx = INT64_MIN;    ///< Maximum receive time.
    int64_t min_rx = INT64_MAX;    ///< Minimum receive time.

    int64_t max_tx = INT64_MIN;    ///< Maximum transmit time.
    int64_t min_tx = INT64_MAX;    ///< Minimum transmit time.

    const int64_t STATS_INTERVAL_ = 1000;     ///< Statistics interval in milliseconds.
    const int64_t KORD_REF_PERIOD_ = 4000000; ///< Reference period in nanoseconds.

    const int64_t MAX_TX_PACKETS_DIFF_ = KORD_REF_PERIOD_ + KORD_REF_PERIOD_ / 2; ///< Maximum difference for transmit packets.

    uint64_t recents_buffer_size_ = STATS_INTERVAL_ / (KORD_REF_PERIOD_ / 1000000); ///< Size of the recent buffers.

    std::queue<int> jitters_buffer;             ///< Buffer storing jitter values.
    std::queue<int> lost_packets_buffer_cbun;  ///< Buffer storing lost CBun packets.
    std::queue<int> lost_packets_buffer_api;    ///< Buffer storing lost API packets.

    long sum_jitter = 0;     ///< Sum of jitter values.
    long sum_lost_cbun = 0;  ///< Sum of lost CBun packets.
    long sum_lost_api = 0;   ///< Sum of lost API packets.

    int last_received_seq_number_ = -1; ///< Last received sequence number.
    int64_t local_packets_lost_seq = 0; ///< Locally lost sequence packets.
    int64_t global_packets_lost_seq = 0;///< Globally lost sequence packets.

    int64_t global_cons_packets_lost_max = 0; ///< Maximum consecutive lost packets globally.

    std::queue<int> stats_sequences_buffer; ///< Buffer storing sequence statistics.
};

/**
 * @brief Namespace containing real-time related utility functions.
 */
namespace realtime {

    /**
     * @brief Sets the latency target for real-time operations.
     *
     * @return True if the latency target was successfully set, false otherwise.
     */
    bool set_latency_target();

    /**
     * @brief Initializes real-time parameters with the given priority.
     *
     * @param a_prio The priority level to set.
     * @return True if initialization was successful, false otherwise.
     */
    bool init_realtime_params(int a_prio);

} // namespace realtime

/**
 * @brief Converts Euler angles to a quaternion.
 *
 * @param x The X position.
 * @param y The Y position.
 * @param z The Z position.
 * @param roll The roll angle in radians.
 * @param pitch The pitch angle in radians.
 * @param yaw The yaw angle in radians.
 * @return An array of doubles representing the quaternion [w, x, y, z].
 */
std::array<double, 7> convertEulerToQuaternion(double x, double y, double z, double roll, double pitch, double yaw);

} // namespace kr2::utils

#endif // KR2_KORD_UTILS_H
