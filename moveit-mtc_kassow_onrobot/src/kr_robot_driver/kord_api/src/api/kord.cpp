/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2022, Kassow Robots
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

// #include <boost/crc.hpp>
#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS

#include <kord/CRC.h>
#include <kord/api/kord.h>
#include <kord/protocol/DataFormatDescription.h>
#include <kord/protocol/KORDDataIDs.h>
#include <kord/protocol/KORDFrames.h>
#include <kord/protocol/KORDItemIDs.h>
#include <kord/protocol/ServerParameters.h>
#include <kord/protocol/version.h>
#include <kord/utils/logger.h>
#include <kord/utils/timex.h>
#include <kord/version.h>

#include <bitset>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sys/types.h>

#define KORD_UDP_PORT 7582
#define KORD_TIME_OUT 500

using namespace kr2::kord;
using namespace kr2::kord::protocol;

/**
 * @class Internals
 * @brief Internal class for managing status frame parsing.
 *
 * The `Internals` class encapsulates the functionality required to parse status frames
 * within the KORD protocol. It manages frame population, response buffering, content
 * parsing, and synchronization mechanisms.
 */
class KordCore::Internals {
public:
    /**
     * @brief Default constructor.
     *
     * Initializes an `Internals` instance with default settings.
     */
    Internals() = default;

    /**
     * @brief Constructs Internals with a specific session ID.
     *
     * @param session_id The session identifier to associate with the parser.
     */
    explicit Internals(unsigned int session_id) : session_id_(session_id) {}

    /**
     * @brief Populates a frame with content from a content builder.
     *
     * This templated function sets up the frame's header information and retrieves
     * the payload from the provided content builder to populate the frame's payload.
     *
     * @tparam FrameType The type of the frame to populate.
     * @tparam ContentBuilderType The type of the content builder providing the payload.
     * @param frame The frame object to populate with header and payload data.
     * @param content_builder The content builder used to generate the payload data.
     */
    template <typename FrameType, typename ContentBuilderType>
    void populateFrameFromContent(FrameType &frame, ContentBuilderType &content_builder)
    {
        frame.frame_id_ = htons(KORD_FRAME_ID_CONTENT);
        frame.kord_version_ = htons(KORD_PROTOCOL_VERSION);
        frame.session_id_ = htons(session_id_);
        frame.tx_timestamp_ = std::chrono::system_clock::now().time_since_epoch().count();
        size_t content_frm_size = sizeof(FrameType) - KORD_FRAME_HDR_LEN;
        frame.payload_length_ = htons(content_frm_size);
        content_builder.getPayload(frame.payload_, sizeof(frame.payload_));
    }

    /**
     * @brief Destructor.
     *
     * Cleans up resources used by the `Internals` instance.
     */
    ~Internals() = default;

    /**
     * @brief Copy constructor.
     *
     * Creates a new `Internals` instance as a copy of an existing one.
     *
     * @param other The `Internals` instance to copy from.
     */
    Internals(const Internals &other);

    /**
     * @brief Assignment operator.
     *
     * Assigns the contents of one `Internals` instance to another.
     *
     * @param other The `Internals` instance to assign from.
     * @return Reference to the assigned `Internals` instance.
     */
    Internals &operator=(const Internals &other);

    /**
     * @brief Sets the parser's state from the provided payload data.
     *
     * Initializes the parser with the given payload data, allowing it to extract
     * relevant information such as frequency, version, and sequence number.
     *
     * @param data Pointer to the payload data buffer.
     * @param data_len The length of the payload data in bytes.
     * @return `true` if the payload was successfully set and parsed; `false` otherwise.
     */
    bool setFromPayload(uint8_t *data, size_t data_len);

    /**
     * @brief Retrieves the frequency from the parsed status frame.
     *
     * @return The frequency as an `unsigned int`.
     */
    [[nodiscard]] unsigned int getFrequency() const;

    /**
     * @brief Retrieves the version from the parsed status frame.
     *
     * @return The version as an `unsigned int`.
     */
    [[nodiscard]] unsigned int getVersion() const;

    /**
     * @brief Retrieves the sequence number from the parsed status frame.
     *
     * @return The sequence number as an `unsigned int`.
     */
    [[nodiscard]] unsigned int getSequenceNumber() const;

    /**
     * @brief Clears the parser's state.
     *
     * Resets all internal data, allowing the parser to handle new frames.
     */
    void clear();

    KORDFrame eth_kord_proto_frm_{};    /**< Ethernet KORD protocol frame */
    KORDFrameTCP tcp_kord_proto_frm_{}; /**< TCP KORD protocol frame */

    std::vector<KORDFrameTCP> response_frm_buffer_{};   /**< Buffer for TCP KORD response frames */
    std::vector<protocol::Response> response_buffer_{}; /**< Buffer for protocol responses */
    StatusFrameParser status_parser_{};                 /**< Parser for status frames */

    DataFormatDescription status_dfd_ =
        DataFormatDescription::makeStatusFrameDescription(); /**< Data format description for status frames */

    ContentFrameBuilder<KORDContentFrame> content_builder_{};        /**< Builder for KORD content frames */
    ContentFrameBuilder<KORDContentFrameTCP> content_builder_tcp_{}; /**< Builder for KORD TCP content frames */
    ContentFrameParser<KORDContentFrame> content_parser_{};          /**< Parser for KORD content frames */
    ContentFrameParser<KORDContentFrameTCP> content_parser_tcp_{};   /**< Parser for KORD TCP content frames */

    // clang-format off
    ContentItem<MAX_UDP_DATA_LEN> rx_content_items_[MAX_CONTENT_ITEMS]{}; /**< Receive content items for UDP */
    ContentItem<MAX_UDP_DATA_LEN> tx_content_items_[MAX_CONTENT_ITEMS]{}; /**< Transmit content items for UDP */

    ContentItem<MAX_TCP_DATA_LEN> rx_content_items_tcp_[MAX_CONTENT_ITEMS]{}; /**< Receive content items for TCP */
    ContentItem<MAX_TCP_DATA_LEN> tx_content_items_tcp_[MAX_CONTENT_ITEMS]{}; /**< Transmit content items for TCP */

    DataFormatDescription req_arm_status_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eRequestStatusArm); /**< Data format description for arm status request */
    DataFormatDescription cmd_joint_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveJ); /**< Data format description for joint movement command */
    DataFormatDescription cmd_linear_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveL); /**< Data format description for linear movement command */
    DataFormatDescription cmd_linear_quat_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveLQuat); /**< Data format description for linear movement with quaternion command */
    DataFormatDescription cmd_manifold_dfd_   = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandManifold); /**< Data format description for manifold command */
    DataFormatDescription cmd_direct_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveDirect); /**< Data format description for direct movement command */
    DataFormatDescription cmd_direct_torque_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveDirectTorque); /**< Data format description for direct movement with torque command */
    DataFormatDescription cmd_velocity_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveVelocity); /**< Data format description for velocity movement command */
    DataFormatDescription cmd_joint_fw_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandJointFirmware); /**< Data format description for joint firmware command */
    DataFormatDescription cmd_setFrame_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandSetFrame); /**< Data format description for setting frame parameters */
    DataFormatDescription cmd_setLoad_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandSetLoad); /**< Data format description for setting load parameters */
    DataFormatDescription cmd_cleanAlarm_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandCleanAlarm); /**< Data format description for cleaning alarms */
    DataFormatDescription cmd_rc_api_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandRCAPI); /**< Data format description for RC API command */
    DataFormatDescription req_sys_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eRequestSystem); /**< Data format description for system status request */
    DataFormatDescription req_transf_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eRequestTransfer); /**< Data format description for data transfer request */
    DataFormatDescription res_statusEcho_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eStatusEchoResponse); /**< Data format description for status echo response */
    DataFormatDescription req_io_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandSetIODigitalOut); /**< Data format description for setting I/O digital output */
    DataFormatDescription req_server_dfd_ = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eRequestServer); /**< Data format description for server request */
    unsigned int req_sys_seq_num_{0}; /**< Sequence number for system requests */
    // clang-format on

    RobotArmStatus sync_arm_status_;        /**< Status of the synchronized robot arm */
    CommandStatuses sync_command_statuses_; /**< Statuses of synchronized commands */

    std::mutex comm_mutex_; /**< Mutex for communication operations */
    std::mutex read_mutex_; /**< Mutex for read operations */

    unsigned int session_id_{0}; /**< Session identifier */
};

KordCore::KordCore(const std::string &a_hostname, unsigned int a_port, unsigned int a_session_id, connection conn)
    : his_(new Internals(a_session_id))
{
    conn_type_ = conn;
    switch (conn) {
    case TCP_SERVER:
        conn_container_ = std::make_shared<TCPServer>(io_service_, a_port);
        break;
    case TCP_CLIENT:
        conn_container_ = std::make_shared<TCPClient>(io_service_, a_hostname, a_port);
        break;
    case UDP_SERVER:
        conn_container_ = std::make_shared<UDPServer>(io_service_, a_hostname, a_port);
        break;
    case UDP_CLIENT:
        conn_container_ = std::make_shared<UDPClient>(io_service_, a_hostname, a_port);
        break;
    }

    conn_container_requests_ = std::make_shared<TCPClient>(io_requests_service_, a_hostname, a_port + 1);

    hostname_ = a_hostname;
    port_ = a_port;
    session_id_ = a_session_id;

    KORD_LOG_INFO("KORD API version: " << API_VERSION << ", sha: " << API_SHA1);
    KORD_LOG_INFO("KORD Protocol version: " << PROTOCOL_VERSION << ", sha: " << PROTOCOL_SHA1);
}

KordCore::~KordCore()
{
    disconnect();
    delete his_;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::asPoseControl()
{
    this->command_type = KordCore::RobotFrameCommand::MOVE_POSE;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::asPoseQuatControl()
{
    this->command_type = KordCore::RobotFrameCommand::MOVE_POSE_QUAT;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::asVelocityControl()
{
    this->command_type = KordCore::RobotFrameCommand::MOVE_VELOCITY;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::asPosVelControl()
{
    this->command_type = KordCore::RobotFrameCommand::MOVE_POSE_DYN;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withTargetPose(const std::array<double, 6UL> &a_pose)
{
    this->tcp_target_ = a_pose;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withTargetPoseQuat(const std::array<double, 7UL> &a_pose_quat)
{
    this->tcp_quat_target_ = a_pose_quat;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withTargetVelocity(const std::array<double, 6UL> &a_velocity)
{
    this->tcp_target_velocity_ = a_velocity;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withSyncValue(const long &a_sync_value)
{
    this->sync_value_ = a_sync_value;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withPeriod(const double &a_period)
{
    this->period_ = a_period;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withTimeout(const double &a_timeout)
{
    this->timeout_ = a_timeout;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withTargetAcceleration(
    const std::array<double, 6UL> &a_acceleration)
{
    this->tcp_target_acc_ = a_acceleration;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withTrackingType(const TrackingType &a_tt)
{
    this->tt_ = a_tt;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withTrackingValue(const double &a_tt_value)
{
    this->tt_value_ = a_tt_value;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withBlendType(const BlendType &a_bt)
{
    this->bt_ = a_bt;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withBlendValue(const double &a_bt_value)
{
    this->bt_value_ = a_bt_value;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withOverlayType(const OverlayType &a_ot)
{
    this->ot_ = a_ot;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::withSyncTime(const double &a_sync_time)
{
    this->sync_time_ = a_sync_time;
    return *this;
}

KordCore::RobotFrameCommand &KordCore::RobotFrameCommand::setSequenceNumber(unsigned int a_seq)
{
    this->seq_ = a_seq;
    return *this;
}

bool KordCore::connect(const char *device)
{
    using namespace std::chrono_literals;

    kr2::utils::Timex::initSingletonTimex(500, false);

    memset(&his_->eth_kord_proto_frm_, 0x00, sizeof(KORDFrame));
    memset(&his_->tcp_kord_proto_frm_, 0x00, sizeof(KORDFrameTCP));

    if (!conn_container_->connect(device)) {
        KORD_LOG_ERROR("Failed to connect to the server.");
        return false;
    }

    if (!conn_container_requests_->connect(device)) {
        KORD_LOG_ERROR("Failed to connect to the requests server.");
        return false;
    }

    // stats reset
    start_ = std::chrono::steady_clock::now();
    packets_cnt_ = 0;
    sequence_number_ = 0;
    return true;
}

bool KordCore::disconnect() { return conn_container_->disconnect(); }

bool KordCore::syncRC(long flags)
{
    if (!conn_container_->isConnected()) {
        return false;
    }
    requestArmStatus();
    int time_out_counter = 0;    // counter for time out procedure
    bool full_cycle = flags & 1; // first bit of flags responsible for full cycle rotation

    bool got_first_id = false;
    bool second_loop = false;
    int first_frame_id = -1;
    int now_frame_id = -1;
    int first_load_id = -1;
    int now_load_id = -1;
    int first_cpu_state_id = -1;
    int now_cpu_state_id = -1;

    do {
        do {
            time_out_counter++;
            do {
                auto ts_sent = std::chrono::steady_clock::now();
                ts_record_.t0_ = ts_sent.time_since_epoch().count();

                memset(&his_->eth_kord_proto_frm_, 0x0, sizeof(KORDFrame));

                [[maybe_unused]] uint n = conn_container_->recvFrame(&his_->eth_kord_proto_frm_);
                /*
                 * Waiting for the first timed response marking the RC start of the
                 * tick. Calculate every other tick starting captured update.
                 *
                 * DEBUG: To observe the timeout counter uncomment the following line.
                 */
                time_out_counter++;
            } while (his_->eth_kord_proto_frm_.frame_id_ != KORD_FRAME_ID_STATUS && time_out_counter < KORD_TIME_OUT);
            if (time_out_counter >= KORD_TIME_OUT) {
                KORD_LOG_ERROR("SyncRC timed out.");
                return false;
            }
        } while (his_->eth_kord_proto_frm_.session_id_ != session_id_);

        if (!his_->status_parser_.setFromPayload(his_->eth_kord_proto_frm_.payload_,
                                                 his_->eth_kord_proto_frm_.payload_length_)) {
            KORD_LOG_ERROR("Failed to set the Status frame payload.");
            KORD_LOG_ERROR("KORD Payload -     length: " << his_->eth_kord_proto_frm_.payload_length_);
            KORD_LOG_ERROR("KORD Payload -   frame_id: " << his_->eth_kord_proto_frm_.frame_id_);
            KORD_LOG_ERROR("KORD Payload - session_id: " << his_->eth_kord_proto_frm_.session_id_);
            return false;
        }

        // if we closed the cycle, no need to read
        if (!got_first_id || !second_loop || first_frame_id != now_frame_id) {
            now_frame_id = getIterativeFrameId();
        }

        if (!got_first_id || !second_loop || first_load_id != now_load_id) {
            now_load_id = getIterativeLoadId();
        }

        if (!got_first_id || !second_loop || first_cpu_state_id != now_cpu_state_id) {
            now_cpu_state_id = getIterativeCPUStateId();
        }

        if (!got_first_id) {
            first_frame_id = now_frame_id;
            first_load_id = now_load_id;
            first_cpu_state_id = now_cpu_state_id;
            got_first_id = true;
        }
        else {
            second_loop = true;
        }
        // storing new values
        updateRecentArmStatus(his_->sync_arm_status_);

        ctlrc_update_ts_ = kr2::utils::Timex::thisTick(kr2::utils::Timex::now());
    } while (full_cycle && (!second_loop || !((first_frame_id == now_frame_id) && (first_load_id == now_load_id) &&
                                              (first_cpu_state_id == now_cpu_state_id))));

    return true;
}

bool KordCore::waitSync(std::chrono::microseconds a_timeout_us, long flags)
{
    std::lock_guard guard(his_->read_mutex_);

    int time_out_counter = 0;
    bool full_cycle = flags & 1;
    bool only_recent = (flags >> 1) & 1;
    const auto recent_interval = std::chrono::nanoseconds(std::chrono::milliseconds(6));
    bool not_recent = false;

    bool got_first_id = false;
    bool second_loop = false;
    int first_frame_id = -1;
    int now_frame_id = -1;
    int first_load_id = -1;
    int now_load_id = -1;
    int first_cpu_state_id = -1;
    int now_cpu_state_id = -1;

    // Receiving RSHB
    while (true) {
        if (!waitForValidFrame(a_timeout_us, time_out_counter)) {
            return false;
        }

        parseFrame();

        if (!updateIds(got_first_id,
                       second_loop,
                       first_frame_id,
                       now_frame_id,
                       first_load_id,
                       now_load_id,
                       first_cpu_state_id,
                       now_cpu_state_id)) {
            continue;
        }

        updateRecentStatuses();

        if (only_recent) {
            not_recent = !isRecent(recent_interval);
        }

        stats.capture(std::chrono::steady_clock::now(), getTxStamp(), his_->status_parser_.getSequenceNumber());
        ctlrc_update_ts_ = utils::Timex::thisTick(kr2::utils::Timex::now());

        if (!shouldContinueLoop(not_recent,
                                full_cycle,
                                second_loop,
                                first_frame_id,
                                now_frame_id,
                                first_load_id,
                                now_load_id,
                                first_cpu_state_id,
                                now_cpu_state_id)) {
            break;
        }
    }

    // Receiving TCP responses
    receiveTCPFrame();

    return true;
}

bool KordCore::waitForValidFrame(std::chrono::microseconds a_timeout_us, int &time_out_counter)
{
    while (true) {
        time_out_counter++;
        auto ts_start = std::chrono::steady_clock::now();
        while (true) {
            memset(&his_->eth_kord_proto_frm_, 0x0, sizeof(KORDFrame));
            conn_container_->recvFrame(&his_->eth_kord_proto_frm_);

            if (hasTimedOut(ts_start, a_timeout_us)) {
                stats.update_failure();
                return false;
            }

            if (his_->eth_kord_proto_frm_.frame_id_ == KORD_FRAME_ID_STATUS ||
                his_->eth_kord_proto_frm_.frame_id_ == KORD_FRAME_ID_CONTENT) {
                break;
            }
        }

        if (time_out_counter > KORD_TIME_OUT) {
            KORD_LOG_ERROR("Time out.");
            return false;
        }

        if (his_->eth_kord_proto_frm_.session_id_ == session_id_) {
            return true;
        }
    }
}

bool KordCore::hasTimedOut(const std::chrono::steady_clock::time_point &start, const std::chrono::microseconds &timeout)
{
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start);
    return elapsed.count() > timeout.count();
}

void KordCore::parseFrame()
{
    if (his_->eth_kord_proto_frm_.frame_id_ == KORD_FRAME_ID_STATUS) {
        his_->status_parser_.setFromPayload(his_->eth_kord_proto_frm_.payload_,
                                            his_->eth_kord_proto_frm_.payload_length_);
    }
    else if (his_->eth_kord_proto_frm_.frame_id_ == KORD_FRAME_ID_CONTENT) {
        his_->content_parser_.setFromPayload(his_->eth_kord_proto_frm_.payload_,
                                             his_->eth_kord_proto_frm_.payload_length_);
    }
}

void KordCore::receiveTCPFrame()
{
    KORDFrameTCP frm{};
    memset(&frm, 0x0, sizeof(KORDFrameTCP));
    auto n = conn_container_requests_->recvFrame(&frm);
    if (n == 0) {
        return;
    }
    if (frm.frame_id_ == KORD_FRAME_ID_CONTENT) {
        his_->content_parser_tcp_.clear();
        his_->content_parser_tcp_.setFromPayload(frm.payload_, frm.payload_length_);
    }
    std::vector<uint8_t> payload(frm.payload_length_);

    token_t token_;
    auto response_dfd = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eResponseSystem);
    unsigned int offset = response_dfd.getOffset(EKORDDataID::eResponseToken);
    std::memcpy(&token_, frm.payload_ + offset, sizeof(token_));

    auto item = his_->content_parser_tcp_.getItemContent(0);
    std::vector<uint8_t> response_payload{item.getItemData(), item.getItemData() + item.getItemDataLength()};
    Response response(response_payload);

    his_->response_frm_buffer_.emplace_back(frm);
    his_->response_buffer_.emplace_back(response);
}

bool KordCore::updateIds(bool &got_first_id,
                         bool &second_loop,
                         int &first_frame_id,
                         int &now_frame_id,
                         int &first_load_id,
                         int &now_load_id,
                         int &first_cpu_state_id,
                         int &now_cpu_state_id) const
{
    bool idsUpdated = false;

    if (!got_first_id || !second_loop || first_frame_id != now_frame_id) {
        now_frame_id = getIterativeFrameId();
        idsUpdated = true;
    }

    if (!got_first_id || !second_loop || first_load_id != now_load_id) {
        now_load_id = getIterativeLoadId();
        idsUpdated = true;
    }

    if (!got_first_id || !second_loop || first_cpu_state_id != now_cpu_state_id) {
        now_cpu_state_id = getIterativeCPUStateId();
        idsUpdated = true;
    }

    if (!got_first_id) {
        first_frame_id = now_frame_id;
        first_load_id = now_load_id;
        first_cpu_state_id = now_cpu_state_id;
        got_first_id = true;
    }
    else {
        second_loop = true;
    }

    return idsUpdated;
}

void KordCore::updateRecentStatuses()
{
    updateRecentArmStatus(his_->sync_arm_status_);
    updateRecentCommandStatus(his_->sync_command_statuses_);
}

bool KordCore::isRecent(const std::chrono::nanoseconds &recent_interval) const
{
    return !(std::chrono::steady_clock::now() - stats.cap_latest_ > recent_interval ||
             getTxStamp() - stats.cap_latest_cbun_ > recent_interval.count());
}

bool KordCore::shouldContinueLoop(bool not_recent,
                                  bool full_cycle,
                                  bool second_loop,
                                  int first_frame_id,
                                  int now_frame_id,
                                  int first_load_id,
                                  int now_load_id,
                                  int first_cpu_state_id,
                                  int now_cpu_state_id)
{
    return not_recent ||
           (full_cycle && (!second_loop || !((first_frame_id == now_frame_id) && (first_load_id == now_load_id) &&
                                             (first_cpu_state_id == now_cpu_state_id))));
}

int64_t KordCore::getTxStamp() const
{
    return his_->status_parser_.getData<int64_t>(his_->status_dfd_.getOffset(EKORDDataID::eTxStamp));
}

uint8_t KordCore::getIterativeFrameId() const
{
    return his_->status_parser_.getData<uint8_t>(his_->status_dfd_.getOffset(EKORDDataID::eFrameId)) << 1 |
           his_->status_parser_.getData<uint8_t>(
               his_->status_dfd_.getOffset(EKORDDataID::eFramePoseRef)); // to get unique id, need to consider
    // reference too
}

uint8_t KordCore::getIterativeLoadId() const
{
    return his_->status_parser_.getData<uint8_t>(his_->status_dfd_.getOffset(EKORDDataID::eLoadId));
}

uint8_t KordCore::getIterativeCPUStateId() const
{
    return his_->status_parser_.getData<uint8_t>(his_->status_dfd_.getOffset(EKORDDataID::eCPUStateId));
}

std::vector<Response> KordCore::getResponses() { return his_->response_buffer_; }

bool KordCore::eraseResponse(token_t token)
{
    his_->response_buffer_.erase(std::remove_if(his_->response_buffer_.begin(),
                                                his_->response_buffer_.end(),
                                                [token](const Response &response) {
                                                    return response.getToken() == token;
                                                }),
                                 his_->response_buffer_.end());

    // TODO: remove this if statement
    if (std::find_if(his_->response_buffer_.begin(), his_->response_buffer_.end(), [token](const Response &response) {
            return response.getToken() == token;
        }) == his_->response_buffer_.end()) {
    }
    else {
        KORD_LOG_ERROR("Response with token " << token << " not erased.");
    }

    return true;
}

void printHorizontalLine(int len) { KORD_LOG_INFO(std::string(len, '-')); }

std::string checkForNoData(const int64_t &a_data, const double &a_factor = 1, const int &precision = 3)
{
    if (a_data == INT64_MIN || a_data == INT64_MAX) {
        return "NA";
    }
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << a_data / a_factor;
    return stream.str();
}

std::string checkPositivity(const double a_value)
{
    if (a_value < 0) {
        return "NA";
    }
    std::ostringstream stream;
    stream << a_value;
    return stream.str();
}

void KordCore::printStats(const CBunReceivedStatistics &a_cbun_stats)
{
    const int columns = 7;           // how many data columns after the first column
    const int columnWidth = 14;      // width for each data column
    const int firstColumnWidth = 35; // total width for the first column
    const int firstColumnPart = 5;   // portion of first column for "CBUN"/"API"

    // A helper to compute total width for the horizontal line for a "columns"-wide table:
    auto computeTableWidth = [&](int numDataCols) {
        return firstColumnWidth + (numDataCols * columnWidth) - 1; // + (numDataCols + 1);
    };

    {
        int totalWidth = computeTableWidth(columns);

        // Print top horizontal line
        printHorizontalLine(totalWidth);

        // Header
        KORD_LOG_INFO(std::left << std::setw(firstColumnWidth) << "Metrics" << "| " << std::setw(columnWidth)
                                << "WindowMax[ms]" << "| " << std::setw(columnWidth) << "WindowAvg[ms]" << "| "
                                << std::setw(columnWidth) << "WindowMin[ms]" << "| " << std::setw(columnWidth)
                                << "GlobalMax[ms]" << "| " << std::setw(columnWidth) << "GlobalAvg[ms]" << "| "
                                << std::setw(columnWidth) << "GlobalMin[ms]" << "|");

        printHorizontalLine(totalWidth);

        KORD_LOG_INFO(std::left << std::setw(firstColumnPart) << " " << std::setw(firstColumnWidth - firstColumnPart)
                                << "| Command Jitter" << "| " << std::setw(columnWidth)
                                << checkPositivity(a_cbun_stats.cmd_jitter_window_max / 1000.) << "| "
                                << std::setw(columnWidth) << checkPositivity(a_cbun_stats.cmd_jitter_window_avg / 1000.)
                                << "| " << std::setw(columnWidth) << "" // no WindowMin
                                << "| " << std::setw(columnWidth)
                                << checkPositivity(a_cbun_stats.cmd_jitter_global_max / 1000.) << "| "
                                << std::setw(columnWidth) << ""         // no GlobalAvg
                                << "| " << std::setw(columnWidth) << "" // no GlobalMin
                                << "|");

        KORD_LOG_INFO(std::left << std::setw(firstColumnPart) << "CBUN" << std::setw(firstColumnWidth - firstColumnPart)
                                << "| Round Trip Time (RTT)" << "| " << std::setw(columnWidth)
                                << checkPositivity(a_cbun_stats.round_trip_window_max / 1000.) << "| "
                                << std::setw(columnWidth) << checkPositivity(a_cbun_stats.round_trip_window_avg / 1000.)
                                << "| " << std::setw(columnWidth) << "" << "| " << std::setw(columnWidth)
                                << checkPositivity(a_cbun_stats.round_trip_global_max / 1000.) << "| "
                                << std::setw(columnWidth) << "" << "| " << std::setw(columnWidth) << "" << "|");

        KORD_LOG_INFO(std::left << std::setw(firstColumnPart) << " " << std::setw(firstColumnWidth - firstColumnPart)
                                << "| System Jitter" << "| " << std::setw(columnWidth)
                                << checkPositivity(a_cbun_stats.system_jitter_window_max / 1000.) << "| "
                                << std::setw(columnWidth)
                                << checkPositivity(a_cbun_stats.system_jitter_window_avg / 1000.) << "| "
                                << std::setw(columnWidth) << "" << "| " << std::setw(columnWidth)
                                << checkPositivity(a_cbun_stats.system_jitter_global_max / 1000.) << "| "
                                << std::setw(columnWidth) << "" << "| " << std::setw(columnWidth) << "" << "|");

        printHorizontalLine(totalWidth);
    }

    {
        int totalWidth = computeTableWidth(columns);

        // Header
        printHorizontalLine(totalWidth);
        KORD_LOG_INFO(std::left << std::setw(firstColumnWidth) << "Metrics" << "| " << std::setw(columnWidth)
                                << "WindowMax[ms]" << "| " << std::setw(columnWidth) << "WindowAvg[ms]" << "| "
                                << std::setw(columnWidth) << "WindowMin[ms]" << "| " << std::setw(columnWidth)
                                << "GlobalMax[ms]" << "| " << std::setw(columnWidth) << "GlobalAvg[ms]" << "| "
                                << std::setw(columnWidth) << "GlobalMin[ms]" << "|");
        printHorizontalLine(totalWidth);

        KORD_LOG_INFO(std::left << std::setw(firstColumnPart) << "" << std::setw(firstColumnWidth - firstColumnPart)
                                << "| API's TimeStamps Delay" << "| " << std::setw(columnWidth) << "" << "| "
                                << std::setw(columnWidth) << "" << "| " << std::setw(columnWidth) << "" << "| "
                                << std::setw(columnWidth) << checkForNoData(stats.get_max_rx(), 1000000.) << "| "
                                << std::setw(columnWidth) << checkForNoData(stats.get_avg_rx(), 1000000.) << "| "
                                << std::setw(columnWidth) << checkForNoData(stats.get_min_rx(), 1000000.) << "|");

        KORD_LOG_INFO(std::left << std::setw(firstColumnPart) << "API" << std::setw(firstColumnWidth - firstColumnPart)
                                << "| CBun's TimeStamps Delay" << "| " << std::setw(columnWidth) << "" << "| "
                                << std::setw(columnWidth) << "" << "| " << std::setw(columnWidth) << "" << "| "
                                << std::setw(columnWidth) << checkForNoData(stats.get_max_tx(), 1000000.) << "| "
                                << std::setw(columnWidth) << checkForNoData(stats.get_avg_tx(), 1000000.) << "| "
                                << std::setw(columnWidth) << checkForNoData(stats.get_min_tx(), 1000000.) << "|");

        KORD_LOG_INFO(std::left << std::setw(firstColumnPart) << "" << std::setw(firstColumnWidth - firstColumnPart)
                                << "| API's TimeStamps Jitter" << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_max_jitter(), 1000000.) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_avg_jitter(), 1000000.) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_min_jitter(), 1000000.) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_max_rx_jitter(), 1000000.) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_avg_rx_jitter(), 1000000.) << "| " << std::setw(columnWidth)
                                << "" << "|");

        printHorizontalLine(totalWidth);
    }

    {
        int lostColumns = 4;
        int totalWidth = firstColumnWidth + lostColumns * (columnWidth + 2) + 1;

        printHorizontalLine(totalWidth);
        KORD_LOG_INFO(std::left << std::setw(firstColumnWidth) << "Metrics" << "| " << std::setw(columnWidth)
                                << "WindowMax" << "| " << std::setw(columnWidth) << "WindowAvg" << "| "
                                << std::setw(columnWidth) << "WindowMin" << "| " << std::setw(columnWidth) << "Overall"
                                << "|");
        printHorizontalLine(totalWidth);

        KORD_LOG_INFO(std::left << std::setw(firstColumnPart) << "" << std::setw(firstColumnWidth - firstColumnPart)
                                << "| API RX Stamp" << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_max_lost_api(), 1, 0) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_avg_lost_api(), 1, 0) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_min_lost_api(), 1, 0) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.sum_lost_api, 1, 0) << "|");

        KORD_LOG_INFO(std::left << std::setw(firstColumnPart) << "API" << std::setw(firstColumnWidth - firstColumnPart)
                                << "| Packet Lost   RSHB TX Stamp" << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_max_lost_cbun(), 1, 0) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_avg_lost_cbun(), 1, 0) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_min_lost_cbun(), 1, 0) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.sum_lost_cbun, 1, 0) << "|");

        KORD_LOG_INFO(std::left << std::setw(firstColumnPart) << "" << std::setw(firstColumnWidth - firstColumnPart)
                                << "| RSHB SEQ" << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_max_lost_seq(), 1, 0) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_avg_lost_seq(), 1, 0) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.get_min_lost_seq(), 1, 0) << "| " << std::setw(columnWidth)
                                << checkForNoData(stats.local_packets_lost_seq, 1, 0) << "|");

        printHorizontalLine(totalWidth);
    }

    {
        int lostColumns = 2;
        int totalWidth = firstColumnWidth + lostColumns * (columnWidth + 2) + 1;

        printHorizontalLine(totalWidth);
        KORD_LOG_INFO(std::left << std::setw(firstColumnWidth) << "Metrics" << "| " << std::setw(columnWidth)
                                << "WindowNumber" << "| " << std::setw(columnWidth) << "GlobalNumber" << "|");
        printHorizontalLine(totalWidth);

        KORD_LOG_INFO(std::left << std::setw(firstColumnPart) << "" << std::setw(firstColumnWidth - firstColumnPart)
                                << "| SEQ" << "| " << std::setw(columnWidth)
                                << checkPositivity(a_cbun_stats.cmd_lost_window_seq) << "| " << std::setw(columnWidth)
                                << checkPositivity(a_cbun_stats.cmd_lost_global_seq) << "|");

        KORD_LOG_INFO(std::left << std::setw(firstColumnPart) << "CBUN" << std::setw(firstColumnWidth - firstColumnPart)
                                << "| Commands Lost" << "| " << std::setw(columnWidth) << "" << "| "
                                << std::setw(columnWidth) << "" << "|");

        KORD_LOG_INFO(
            std::left << std::setw(firstColumnPart) << "" << std::setw(firstColumnWidth - firstColumnPart) << "| Stamp"
                      << "| " << std::setw(columnWidth) << checkPositivity(a_cbun_stats.cmd_lost_window_timestmp) << "| "
                      << std::setw(columnWidth) << checkPositivity(a_cbun_stats.cmd_lost_global_timestmp) << "|");

        printHorizontalLine(totalWidth);
    }

    KORD_LOG_INFO("Number of CBun empty reads: " << a_cbun_stats.fail_to_read_empty);
    KORD_LOG_INFO("Number of times CBun failed to read: " << a_cbun_stats.fail_to_read_error);
    KORD_LOG_INFO("Number of times KORD API failed to receive: " << stats.get_failed_cnt());
}

void KordCore::setStatisticsWindow(int a_window_size) { stats.set_stats_window(a_window_size); }

int64_t KordCore::getAPIStatistics(EAPIStatistics a_value_type)
{
    switch (a_value_type) {
    case MAX_RX_GLOBAL:
        return stats.get_max_rx();
    case MIN_RX_GLOBAL:
        return stats.get_min_rx();
    case AVG_RX_GLOBAL:
        return stats.get_avg_rx();
    case RSHB_JITTER_MAX_LOCAL:
        return stats.get_max_jitter();
    case RSHB_JITTER_MIN_LOCAL:
        return stats.get_min_jitter();
    case RSHB_JITTER_AVG_LOCAL:
        return stats.get_avg_jitter();
    case RSHB_JITTER_AVG_GLOBAL:
        return stats.get_avg_rx_jitter();
    case RSHB_JITTER_MAX_GLOBAL:
        return stats.get_max_rx_jitter();
    case MAX_TX_GLOBAL:
        return stats.get_max_tx();
    case MIN_TX_GLOBAL:
        return stats.get_min_tx();
    case AVG_TX_GLOBAL:
        return stats.get_avg_tx();
    case MAX_LOST_API:
        return stats.get_max_lost_api();
    case MIN_LOST_API:
        return stats.get_min_lost_api();
    case AVG_LOST_API:
        return stats.get_avg_lost_api();
    case MAX_LOST_CBUN:
        return stats.get_max_lost_cbun();
    case MIN_LOST_CBUN:
        return stats.get_min_lost_cbun();
    case AVG_LOST_CBUN:
        return stats.get_avg_lost_cbun();
    case MAX_LOST_SEQ:
    case RSHB_CONS_LOST_COUNTER_MAX_LOCAL:
        return stats.get_max_lost_seq();
    case MIN_LOST_SEQ:
        return stats.get_min_lost_seq();
    case AVG_LOST_SEQ:
    case RSHB_CONS_LOST_COUNTER_AVG_LOCAL:
        return stats.get_avg_lost_seq();
    case LOST_TOTAL_API:
        return stats.sum_lost_api;
    case LOST_TOTAL_CBUN:
        return stats.sum_lost_cbun;
    case LOST_TOTAL_SEQ:
    case RSHB_LOST_COUNTER_LOCAL:
        return stats.local_packets_lost_seq;
    case RSHB_CONS_LOST_COUNTER_MAX_GLOBAL:
        return stats.get_max_drop_global();
    case FAILED_RCV:
        return stats.get_failed_cnt();
    default:
        break;
    }

    return {};
}

void KordCore::spin()
{
    timespec next_tick = kr2::utils::Timex::thisTick(ctlrc_update_ts_, 2);
    kr2::utils::Timex::nanosleepUntil(next_tick);
    ctlrc_update_ts_ = next_tick;
}

void KordCore::makeCommandMoveJ(DefaultContentItem *a_content_item, const RobotArmCommand &a_jcmd)
{
    a_content_item->clear();
    a_content_item->setItemID(EKORDItemID::eCommandMoveJ);
    auto time_now = std::chrono::steady_clock::now().time_since_epoch().count();
    a_content_item->addData<uint16_t>(static_cast<uint16_t>(a_jcmd.seq_),
                                      his_->cmd_joint_dfd_.getOffset(EKORDDataID::eSequenceNumber));
    a_content_item->addData<int64_t>(time_now, his_->cmd_joint_dfd_.getOffset(EKORDDataID::eTxStamp));
    a_content_item->addData<uint8_t>(0x01, his_->cmd_joint_dfd_.getOffset(EKORDDataID::eCTRMovementMask));
    a_content_item->addData<std::array<double, 7>>(a_jcmd.positions_,
                                                   his_->cmd_joint_dfd_.getOffset(EKORDDataID::eJConfigurationArm));
    a_content_item->addData<uint8_t>(a_jcmd.tt_, his_->cmd_joint_dfd_.getOffset(EKORDDataID::eTMovementType));
    a_content_item->addData<double>(a_jcmd.tt_value_, his_->cmd_joint_dfd_.getOffset(EKORDDataID::eTMovementValue));
    a_content_item->addData<uint8_t>(a_jcmd.bt_, his_->cmd_joint_dfd_.getOffset(EKORDDataID::eTBlendType));
    a_content_item->addData<double>(a_jcmd.bt_value_, his_->cmd_joint_dfd_.getOffset(EKORDDataID::eTBlendValue));
    a_content_item->addData<uint8_t>(a_jcmd.ot_, his_->cmd_joint_dfd_.getOffset(EKORDDataID::eTOverlayType));
    a_content_item->addData<double>(a_jcmd.sync_time_, his_->cmd_joint_dfd_.getOffset(EKORDDataID::eTSyncValue));

    auto crc = CRC::Calculate(a_content_item->getItemData(), a_content_item->getItemDataLength(), CRC::CRC_16_MODBUS());
    a_content_item->addData<uint16_t>(crc, his_->cmd_joint_dfd_.getOffset(EKORDDataID::eCRCValue));
}

void KordCore::makeCommandMoveD(DefaultContentItem *a_content_item,
                                const std::array<double, 7UL> &a_j_dcmd,
                                const std::array<double, 7UL> &a_jd_dcmd,
                                const std::array<double, 7UL> &a_jdd_dcmd,
                                const std::array<double, 7UL> &a_torque_dcmd,
                                unsigned int a_seq_num)
{
    a_content_item->clear();
    a_content_item->setItemID(EKORDItemID::eCommandMoveDirect);

    a_content_item->addData<uint16_t>(static_cast<uint16_t>(a_seq_num),
                                      his_->cmd_direct_dfd_.getOffset(EKORDDataID::eSequenceNumber));
    a_content_item->addData<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count(),
                                     his_->cmd_direct_dfd_.getOffset(EKORDDataID::eTxStamp));

    a_content_item->addData<std::array<double, 7>>(a_j_dcmd,
                                                   his_->cmd_direct_dfd_.getOffset(EKORDDataID::eJConfigurationArm));
    a_content_item->addData<std::array<double, 7>>(a_jd_dcmd, his_->cmd_direct_dfd_.getOffset(EKORDDataID::eJSpeedArm));
    a_content_item->addData<std::array<double, 7>>(a_jdd_dcmd,
                                                   his_->cmd_direct_dfd_.getOffset(EKORDDataID::eJAccelerationArm));
    a_content_item->addData<std::array<double, 7>>(a_torque_dcmd,
                                                   his_->cmd_direct_dfd_.getOffset(EKORDDataID::eJTorqueArm));

    auto crc = CRC::Calculate(a_content_item->getItemData(), a_content_item->getItemDataLength(), CRC::CRC_16_MODBUS());
    a_content_item->addData<uint16_t>(crc, his_->cmd_direct_dfd_.getOffset(EKORDDataID::eCRCValue));
}

void KordCore::makeCommandMoveDirectTorque(DefaultContentItem *a_content_item,
                                           const std::array<double, 7UL> &a_torque_dcmd,
                                           unsigned int a_seq_num)
{
    a_content_item->clear();
    a_content_item->setItemID(EKORDItemID::eCommandMoveDirectTorque);

    a_content_item->addData<uint16_t>(static_cast<uint16_t>(a_seq_num),
                                      his_->cmd_direct_torque_dfd_.getOffset(EKORDDataID::eSequenceNumber));

    a_content_item->addData(a_torque_dcmd, his_->cmd_direct_torque_dfd_.getOffset(EKORDDataID::eJTorqueArm));

    auto crc = CRC::Calculate(a_content_item->getItemData(), a_content_item->getItemDataLength(), CRC::CRC_16_MODBUS());
    a_content_item->addData<uint16_t>(crc, his_->cmd_direct_torque_dfd_.getOffset(EKORDDataID::eCRCValue));
}

void KordCore::makeCommandMoveL(DefaultContentItem *a_content_item, const RobotFrameCommand &a_lcmd)
{
    a_content_item->clear();
    a_content_item->setItemID(EKORDItemID::eCommandMoveL);
    auto time_now = std::chrono::steady_clock::now().time_since_epoch().count();
    a_content_item->addData<uint16_t>(a_lcmd.seq_, his_->cmd_linear_dfd_.getOffset(EKORDDataID::eSequenceNumber));
    a_content_item->addData<int64_t>(time_now, his_->cmd_linear_dfd_.getOffset(EKORDDataID::eTxStamp));
    a_content_item->addData<uint8_t>(0x01, his_->cmd_linear_dfd_.getOffset(EKORDDataID::eCTRMovementMask));
    a_content_item->addData<std::array<double, 6UL>>(a_lcmd.tcp_target_,
                                                     his_->cmd_linear_dfd_.getOffset(EKORDDataID::eFrmTCPPose));
    a_content_item->addData<uint8_t>(a_lcmd.tt_, his_->cmd_linear_dfd_.getOffset(EKORDDataID::eTMovementType));
    a_content_item->addData<double>(a_lcmd.tt_value_, his_->cmd_linear_dfd_.getOffset(EKORDDataID::eTMovementValue));
    a_content_item->addData<uint8_t>(a_lcmd.bt_, his_->cmd_linear_dfd_.getOffset(EKORDDataID::eTBlendType));
    a_content_item->addData<double>(a_lcmd.bt_value_, his_->cmd_linear_dfd_.getOffset(EKORDDataID::eTBlendValue));
    a_content_item->addData<uint8_t>(a_lcmd.ot_, his_->cmd_linear_dfd_.getOffset(EKORDDataID::eTOverlayType));
    a_content_item->addData<double>(a_lcmd.sync_time_, his_->cmd_linear_dfd_.getOffset(EKORDDataID::eTSyncValue));

    auto crc = CRC::Calculate(a_content_item->getItemData(), a_content_item->getItemDataLength(), CRC::CRC_16_MODBUS());
    a_content_item->addData<uint16_t>(crc, his_->cmd_linear_dfd_.getOffset(EKORDDataID::eCRCValue));
}

void KordCore::makeCommandMoveLQuat(DefaultContentItem *a_content_item, const RobotFrameCommand &a_lcmd)
{
    a_content_item->clear();
    a_content_item->setItemID(EKORDItemID::eCommandMoveLQuat);
    auto time_now = std::chrono::steady_clock::now().time_since_epoch().count();
    a_content_item->addData<uint16_t>(a_lcmd.seq_, his_->cmd_linear_quat_dfd_.getOffset(EKORDDataID::eSequenceNumber));
    a_content_item->addData<int64_t>(time_now, his_->cmd_linear_quat_dfd_.getOffset(EKORDDataID::eTxStamp));
    a_content_item->addData<uint8_t>(0x01, his_->cmd_linear_quat_dfd_.getOffset(EKORDDataID::eCTRMovementMask));
    a_content_item->addData<std::array<double, 7UL>>(a_lcmd.tcp_quat_target_,
                                                     his_->cmd_linear_quat_dfd_.getOffset(
                                                         EKORDDataID::eFrmTCPQuaternion));
    a_content_item->addData<uint8_t>(a_lcmd.tt_, his_->cmd_linear_quat_dfd_.getOffset(EKORDDataID::eTMovementType));
    a_content_item->addData<double>(a_lcmd.tt_value_,
                                    his_->cmd_linear_quat_dfd_.getOffset(EKORDDataID::eTMovementValue));
    a_content_item->addData<uint8_t>(a_lcmd.bt_, his_->cmd_linear_quat_dfd_.getOffset(EKORDDataID::eTBlendType));
    a_content_item->addData<double>(a_lcmd.bt_value_, his_->cmd_linear_quat_dfd_.getOffset(EKORDDataID::eTBlendValue));
    a_content_item->addData<uint8_t>(a_lcmd.ot_, his_->cmd_linear_quat_dfd_.getOffset(EKORDDataID::eTOverlayType));
    a_content_item->addData<double>(a_lcmd.sync_time_, his_->cmd_linear_quat_dfd_.getOffset(EKORDDataID::eTSyncValue));

    auto crc = CRC::Calculate(a_content_item->getItemData(), a_content_item->getItemDataLength(), CRC::CRC_16_MODBUS());
    a_content_item->addData<uint16_t>(crc, his_->cmd_linear_quat_dfd_.getOffset(EKORDDataID::eCRCValue));
}

void KordCore::makeCommandMoveManifold(DefaultContentItem *a_content_item, const RobotArmCommand &a_cmd)
{
    a_content_item->clear();
    a_content_item->setItemID(EKORDItemID::eCommandManifold);
    auto time_now = std::chrono::steady_clock::now().time_since_epoch().count();

    a_content_item->addData<uint16_t>(a_cmd.seq_, his_->cmd_manifold_dfd_.getOffset(EKORDDataID::eSequenceNumber));
    a_content_item->addData<int64_t>(time_now, his_->cmd_manifold_dfd_.getOffset(EKORDDataID::eTxStamp));
    a_content_item->addData<double>(a_cmd.manifold_joint_speed_,
                                    his_->cmd_manifold_dfd_.getOffset(EKORDDataID::eManifoldJointSpeed));

    auto crc = CRC::Calculate(a_content_item->getItemData(), a_content_item->getItemDataLength(), CRC::CRC_16_MODBUS());
    a_content_item->addData<uint16_t>(crc, his_->cmd_manifold_dfd_.getOffset(EKORDDataID::eCRCValue));
}

void KordCore::makeCommandSetFrame(DefaultContentItem *a_content_item,
                                   const RobotSetupCommand &a_lcmd,
                                   int64_t &a_timeStamp)
{
    a_content_item->clear();
    a_content_item->setItemID(EKORDItemID::eCommandSetFrame);
    auto time_now = std::chrono::steady_clock::now().time_since_epoch().count();
    a_content_item->addData<uint16_t>(static_cast<uint16_t>(a_lcmd.seq_),
                                      his_->cmd_setFrame_.getOffset(EKORDDataID::eSequenceNumber));
    a_content_item->addData<int64_t>(time_now, his_->cmd_setFrame_.getOffset(EKORDDataID::eTxStamp));
    a_timeStamp = time_now;
    a_content_item->addData<uint8_t>(a_lcmd.frame_id_, his_->cmd_setFrame_.getOffset(EKORDDataID::eCTRSetFrameId));
    a_content_item->addData<std::array<double, 6UL>>(a_lcmd.pose_,
                                                     his_->cmd_setFrame_.getOffset(EKORDDataID::eCTRSetFramePose));
    a_content_item->addData<uint8_t>(a_lcmd.ref_frame_, his_->cmd_setFrame_.getOffset(EKORDDataID::eCTRSetFrameRef));

    auto crc = CRC::Calculate(a_content_item->getItemData(), a_content_item->getItemDataLength(), CRC::CRC_16_MODBUS());
    a_content_item->addData<uint16_t>(crc, his_->cmd_setFrame_.getOffset(EKORDDataID::eCRCValue));
}

void KordCore::makeCommandSetLoad(DefaultContentItem *a_content_item,
                                  const RobotSetupCommand &a_lcmd,
                                  int64_t &a_timeStamp)
{
    a_content_item->clear();
    a_content_item->setItemID(EKORDItemID::eCommandSetLoad);
    auto time_now = std::chrono::steady_clock::now().time_since_epoch().count();
    a_content_item->addData<uint16_t>(static_cast<uint16_t>(a_lcmd.seq_),
                                      his_->cmd_setLoad_.getOffset(EKORDDataID::eSequenceNumber));
    a_content_item->addData<int64_t>(time_now, his_->cmd_setLoad_.getOffset(EKORDDataID::eTxStamp));
    a_timeStamp = time_now;
    a_content_item->addData<uint8_t>(a_lcmd.load_id_, his_->cmd_setLoad_.getOffset(EKORDDataID::eCTRSetLoadId));
    a_content_item->addData<double>(a_lcmd.load_mass_, his_->cmd_setLoad_.getOffset(EKORDDataID::eCTRSetLoadMass));
    a_content_item->addData<std::array<double, 3UL>>(a_lcmd.load_cog_,
                                                     his_->cmd_setLoad_.getOffset(EKORDDataID::eCTRSetLoadCoG));
    a_content_item->addData<std::array<double, 6UL>>(a_lcmd.load_inertia_,
                                                     his_->cmd_setLoad_.getOffset(EKORDDataID::eCTRSetLoadInertia));

    auto crc = CRC::Calculate(a_content_item->getItemData(), a_content_item->getItemDataLength(), CRC::CRC_16_MODBUS());
    a_content_item->addData<uint16_t>(crc, his_->cmd_setLoad_.getOffset(EKORDDataID::eCRCValue));
}

void KordCore::makeCommandCleanAlarm(DefaultContentItem *a_content_item,
                                     const RobotSetupCommand &a_lcmd,
                                     int64_t &a_timeStamp)
{
    a_content_item->clear();
    a_content_item->setItemID(EKORDItemID::eCommandCleanAlarm);
    auto time_now = std::chrono::steady_clock::now().time_since_epoch().count();
    a_content_item->addData<uint16_t>(static_cast<uint16_t>(a_lcmd.seq_),
                                      his_->cmd_cleanAlarm_.getOffset(EKORDDataID::eSequenceNumber));
    a_content_item->addData<int64_t>(time_now, his_->cmd_cleanAlarm_.getOffset(EKORDDataID::eTxStamp));
    a_timeStamp = time_now;
    a_content_item->addData<uint8_t>(a_lcmd.alarm_id_, his_->cmd_cleanAlarm_.getOffset(EKORDDataID::eCTRCleanAlarmID));

    auto crc = CRC::Calculate(a_content_item->getItemData(), a_content_item->getItemDataLength(), CRC::CRC_16_MODBUS());
    a_content_item->addData<uint16_t>(crc, his_->cmd_cleanAlarm_.getOffset(EKORDDataID::eCRCValue));
}

void KordCore::makeCommandMoveDynL(DefaultContentItem *, const std::array<double, 6UL> &, unsigned int)
{
    // cmdl_.reset();
    // boost::crc_optimal<16, 0x8005, 0xFFFF, 0, true, true> crc;

    // a_out_eth_frame->frame_id_ = htons(static_cast<uint16_t>(KORD_REQ_MOVEL_DYN));
    // a_out_eth_frame->session_id_ = 0;
    // a_out_eth_frame->payload_length_ = htons(cmdl_.getLength());
    // cmdl_.sequence_number_ = static_cast<uint16_t>(a_seq_num);
    // cmdl_.setTXStampNSec(std::chrono::system_clock::now().time_since_epoch().count());

    // //TODO:
    // cmdl_.setTCPTarget(a_in_lcmd.data());

    // void setTCPTarget(const double pose[6]);
    // void setTCPVelocityTarget(const double pose[6]);
    // void setTCPAccelerationTarget(const double pose[6]);

    // crc.process_bytes(a_out_eth_frame, kord::KORDFrame::header_len_);
    // crc.process_bytes(&cmdl_, cmdl_.getLength() - 2);  // process bytes without CRC
    // cmdl_.crc_ = crc.checksum();
}

void KordCore::makeCommandMoveVelocityL(DefaultContentItem *a_content_item,
                                        const std::array<double, 6UL> &a_vels,
                                        unsigned int a_seq_num,
                                        long a_sync_value,
                                        double a_period,
                                        double a_timeout)
{
    a_content_item->clear();
    a_content_item->setItemID(EKORDItemID::eCommandMoveVelocity);
    auto time_now = std::chrono::steady_clock::now().time_since_epoch().count();
    a_content_item->addData<uint16_t>(a_seq_num, his_->cmd_velocity_dfd_.getOffset(EKORDDataID::eSequenceNumber));
    a_content_item->addData<int64_t>(time_now, his_->cmd_velocity_dfd_.getOffset(EKORDDataID::eTxStamp));
    a_content_item->addData<std::array<double, 6UL>>(a_vels,
                                                     his_->cmd_velocity_dfd_.getOffset(EKORDDataID::eFrmTCPVelocity));
    a_content_item->addData<uint32_t>(a_sync_value, his_->cmd_velocity_dfd_.getOffset(EKORDDataID::eVelMoveSync));
    a_content_item->addData<double>(a_period, his_->cmd_velocity_dfd_.getOffset(EKORDDataID::eVelMovePeriod));
    a_content_item->addData<double>(a_timeout, his_->cmd_velocity_dfd_.getOffset(EKORDDataID::eVelMoveTimeout));

    auto crc = CRC::Calculate(a_content_item->getItemData(), a_content_item->getItemDataLength(), CRC::CRC_16_MODBUS());
    a_content_item->addData<uint16_t>(crc, his_->cmd_velocity_dfd_.getOffset(EKORDDataID::eCRCValue));
}

void KordCore::makeArmStatusRequest(DefaultContentItem *a_content_item)
{
    a_content_item->clear();
    a_content_item->setItemID(EKORDItemID::eRequestStatusArm);
    a_content_item->addData<uint16_t>(0x0, his_->req_arm_status_dfd_.getOffset(EKORDDataID::eSequenceNumber));
    a_content_item->addData<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count(),
                                     his_->req_arm_status_dfd_.getOffset(EKORDDataID::eTxStamp));
    a_content_item->addData<uint16_t>(0xFFFF, his_->req_arm_status_dfd_.getOffset(EKORDDataID::eCRCValue));
}

void KordCore::makeCommandFirmware(DefaultContentItem *a_content_item,
                                   const std::array<EJointFirmwareCommand, 7UL> &a_in_fw_cmd,
                                   unsigned int a_seq_num,
                                   int64_t &a_timeStamp)
{
    int64_t tx_ts = std::chrono::steady_clock::now().time_since_epoch().count();
    
    a_content_item->clear();
    a_content_item->setItemID(EKORDItemID::eCommandJointFirmware);
    a_content_item->addData<uint16_t>(a_seq_num, his_->cmd_joint_fw_dfd_.getOffset(EKORDDataID::eSequenceNumber));
    a_content_item->addData<int64_t>(tx_ts, his_->cmd_joint_fw_dfd_.getOffset(EKORDDataID::eTxStamp));
    a_timeStamp = tx_ts;
    a_content_item->addData(a_in_fw_cmd, his_->cmd_joint_fw_dfd_.getOffset(EKORDDataID::eJControlCMD));

    auto crc = CRC::Calculate(a_content_item->getItemData(), a_content_item->getItemDataLength(), CRC::CRC_16_MODBUS());
    a_content_item->addData<uint16_t>(crc, his_->cmd_joint_fw_dfd_.getOffset(EKORDDataID::eCRCValue));
}

int KordCore::setRequestContentItem(DefaultContentItem *a_content, const Request &a_request, unsigned short a_seq)
{
    if (!a_content)
        return -1;

    a_content->clear();

    switch (a_request.system_request_type_) {
    case eRCAPICommand: {
        auto rc_request = dynamic_cast<const RequestRCAPICommand &>(a_request);
        a_content->setItemID(EKORDItemID::eCommandRCAPI);

        // a_content->addData<uint16_t>(a_seq, his_->cmd_rc_api_.getOffset(EKORDDataID::eSequenceNumber));
        a_content->addData<uint64_t>(rc_request.request_rid_, his_->cmd_rc_api_.getOffset(EKORDDataID::eTxStamp));
        a_content->addData<uint16_t>(rc_request.command_id_, his_->cmd_rc_api_.getOffset(EKORDDataID::eRCAPICommandID));
        a_content->addData<uint32_t>(rc_request.payload_length_,
                                     his_->cmd_rc_api_.getOffset(EKORDDataID::eRCAPICommandLength));

        a_content->addData(rc_request.payload_, his_->cmd_rc_api_.getOffset(EKORDDataID::eRCAPICommandPayload));

        auto crc = CRC::Calculate(a_content->getItemData(), a_content->getItemDataLength(), CRC::CRC_16_MODBUS());
        a_content->addData<uint16_t>(crc, his_->cmd_rc_api_.getOffset(EKORDDataID::eCRCValue));
        return 0;
    }
    case eTransferLogFiles: {
        KORD_LOG_DEBUG("Request Retrieve Log File");

        auto system_request = dynamic_cast<const RequestSystem &>(a_request);
        a_content->setItemID(EKORDItemID::eRequestSystem);

        a_content->addData<uint16_t>(a_seq, his_->req_sys_dfd_.getOffset(EKORDDataID::eSequenceNumber));
        a_content->addData<int64_t>(system_request.request_rid_, his_->req_sys_dfd_.getOffset(EKORDDataID::eTxStamp));
        a_content->addData<uint16_t>(system_request.system_request_type_,
                                     his_->req_sys_dfd_.getOffset(EKORDDataID::eCTRCommandItem));

        std::uint32_t crc =
            CRC::Calculate(a_content->getItemData(), a_content->getItemDataLength(), CRC::CRC_16_MODBUS());
        a_content->addData<uint16_t>(crc, his_->req_sys_dfd_.getOffset(EKORDDataID::eCRCValue));

        return 0;
    }
    case eTransferDashboardJson: {
        KORD_LOG_DEBUG("Request Retrieve Dashboard Json Files");

        auto system_request = dynamic_cast<const RequestSystem &>(a_request);
        a_content->setItemID(EKORDItemID::eRequestSystem);

        a_content->addData<uint16_t>(a_seq, his_->req_sys_dfd_.getOffset(EKORDDataID::eSequenceNumber));
        a_content->addData<int64_t>(system_request.request_rid_, his_->req_sys_dfd_.getOffset(EKORDDataID::eTxStamp));
        a_content->addData<uint16_t>(system_request.system_request_type_,
                                     his_->req_sys_dfd_.getOffset(EKORDDataID::eCTRCommandItem));

        auto crc = CRC::Calculate(a_content->getItemData(), a_content->getItemDataLength(), CRC::CRC_16_MODBUS());
        a_content->addData<uint16_t>(crc, his_->req_sys_dfd_.getOffset(EKORDDataID::eCRCValue));

        return 0;
    }
    case eTransferCalibrationData: {
        KORD_LOG_DEBUG("Request Retrieve Calibration Data Files");

        auto system_request = dynamic_cast<const RequestSystem &>(a_request);
        a_content->setItemID(EKORDItemID::eRequestSystem);

        a_content->addData<uint16_t>(a_seq, his_->req_sys_dfd_.getOffset(EKORDDataID::eSequenceNumber));
        a_content->addData<int64_t>(system_request.request_rid_, his_->req_sys_dfd_.getOffset(EKORDDataID::eTxStamp));
        a_content->addData<uint16_t>(system_request.system_request_type_,
                                     his_->req_sys_dfd_.getOffset(EKORDDataID::eCTRCommandItem));

        auto crc = CRC::Calculate(a_content->getItemData(), a_content->getItemDataLength(), CRC::CRC_16_MODBUS());
        a_content->addData<uint16_t>(crc, his_->req_sys_dfd_.getOffset(EKORDDataID::eCRCValue));

        return 0;
    }

    case eTransferFiles: {
        KORD_LOG_DEBUG("Request Retrieve Multiple Files");

        auto transfer_request = dynamic_cast<const RequestTransfer &>(a_request);
        a_content->setItemID(EKORDItemID::eRequestTransfer);
        a_content->addData<uint16_t>(a_seq, his_->req_transf_dfd_.getOffset(EKORDDataID::eSequenceNumber));
        a_content->addData<int64_t>(transfer_request.request_rid_,
                                    his_->req_transf_dfd_.getOffset(EKORDDataID::eTxStamp));
        a_content->addData<uint16_t>(transfer_request.system_request_type_,
                                     his_->req_transf_dfd_.getOffset(EKORDDataID::eCTRCommandItem));

        a_content->addData<uint32_t>(transfer_request.transfer_mask_,
                                     his_->req_transf_dfd_.getOffset(EKORDDataID::eTransferMask));

        auto crc = CRC::Calculate(a_content->getItemData(), a_content->getItemDataLength(), CRC::CRC_16_MODBUS());
        a_content->addData<uint16_t>(crc, his_->req_transf_dfd_.getOffset(EKORDDataID::eCRCValue));

        return 0;
    }
    case eGetVersion: {
        auto version_request = dynamic_cast<const RequestSystem &>(a_request);
        a_content->setItemID(EKORDItemID::eRequestSystem);

        a_content->addData<uint16_t>(a_seq, his_->req_sys_dfd_.getOffset(EKORDDataID::eSequenceNumber));
        a_content->addData<int64_t>(version_request.request_rid_, his_->req_sys_dfd_.getOffset(EKORDDataID::eTxStamp));
        a_content->addData<uint16_t>(version_request.system_request_type_,
                                     his_->req_sys_dfd_.getOffset(EKORDDataID::eCTRCommandItem));

        auto crc = CRC::Calculate(a_content->getItemData(), a_content->getItemDataLength(), CRC::CRC_16_MODBUS());
        a_content->addData<uint16_t>(crc, his_->req_sys_dfd_.getOffset(EKORDDataID::eCRCValue));

        return 0;
    }
    case eServerEnableCommunication:
    case eServerDisableCommunication: {
        auto system_request = dynamic_cast<const RequestSystem &>(a_request);
        a_content->setItemID(EKORDItemID::eRequestSystem);

        a_content->addData<uint16_t>(a_seq, his_->req_sys_dfd_.getOffset(EKORDDataID::eSequenceNumber));
        a_content->addData<int64_t>(system_request.request_rid_, his_->req_sys_dfd_.getOffset(EKORDDataID::eTxStamp));
        a_content->addData<uint16_t>(system_request.system_request_type_,
                                     his_->req_sys_dfd_.getOffset(EKORDDataID::eCTRCommandItem));

        auto crc = CRC::Calculate(a_content->getItemData(), a_content->getItemDataLength(), CRC::CRC_16_MODBUS());
        a_content->addData<uint16_t>(crc, his_->req_sys_dfd_.getOffset(EKORDDataID::eCRCValue));

        return 0;
    }
    case eIOSet: {
        auto io_request = dynamic_cast<const RequestIO &>(a_request);
        a_content->setItemID(EKORDItemID::eCommandSetIODigitalOut);

        a_content->addData<uint16_t>(a_seq, his_->req_io_dfd_.getOffset(EKORDDataID::eSequenceNumber));
        a_content->addData<int64_t>(io_request.request_rid_, his_->req_io_dfd_.getOffset(EKORDDataID::eTxStamp));
        a_content->addData<uint16_t>(io_request.system_request_type_,
                                     his_->req_io_dfd_.getOffset(EKORDDataID::eCTRCommandItem));

        a_content->addData<uint8_t>(io_request.value_, his_->req_io_dfd_.getOffset(EKORDDataID::eIODigitalValue));
        a_content->addData<int64_t>(io_request.mask_, his_->req_io_dfd_.getOffset(EKORDDataID::eIODigitalOutput));

        a_content->addData<int32_t>(io_request.config_id_,
                                    his_->req_io_dfd_.getOffset(EKORDDataID::eIODigitalSafetyConfig));

        // eIOAnalogMask

        auto crc = CRC::Calculate(a_content->getItemData(), a_content->getItemDataLength(), CRC::CRC_16_MODBUS());
        a_content->addData<uint16_t>(crc, his_->req_io_dfd_.getOffset(EKORDDataID::eCRCValue));

        return 0;
    }
    case eServerRequest: {
        auto server_request = dynamic_cast<const RequestServer &>(a_request);
        a_content->setItemID(EKORDItemID::eRequestServer);

        a_content->addData<uint16_t>(a_seq, his_->req_server_dfd_.getOffset(EKORDDataID::eSequenceNumber));
        a_content->addData<int64_t>(server_request.request_rid_, his_->req_server_dfd_.getOffset(EKORDDataID::eTxStamp));

        a_content->addData<uint16_t>(server_request.command_,
                                     his_->req_server_dfd_.getOffset(EKORDDataID::eServerServiceCommand));
        a_content->addData<uint16_t>(server_request.service_id_,
                                     his_->req_server_dfd_.getOffset(EKORDDataID::eServerServiceId));

        // We should encode parameters, so we can send it via KORD, if they are set
        if (server_request.parameters_ != nullptr) {
            std::vector<uint8_t> enc_parameters = server_request.parameters_->dump();
            a_content->addData(enc_parameters, his_->req_server_dfd_.getOffset(EKORDDataID::eServerPayload));
        }

        auto crc = CRC::Calculate(a_content->getItemData(), a_content->getItemDataLength(), CRC::CRC_16_MODBUS());
        a_content->addData<uint16_t>(crc, his_->req_server_dfd_.getOffset(EKORDDataID::eCRCValue));
        return 0;
    }
    default: {
        return -2;
    }
    }
}

int KordCore::setRequestContentItem(TCPContentItem *a_content, const Request &a_request, unsigned short a_seq)
{
    if (!a_content)
        return -1;

    a_content->clear();

    switch (a_request.system_request_type_) {
    case eGetVersion:
    case eGetRobotInfo:
    case eServerEnableCommunication:
    case eServerDisableCommunication:
    case eGetSafetyZones: {
        auto system_request = dynamic_cast<const RequestSystem &>(a_request);
        a_content->setItemID(EKORDItemID::eRequestSystem);

        a_content->addData<uint16_t>(a_seq, his_->req_sys_dfd_.getOffset(EKORDDataID::eSequenceNumber));
        a_content->addData<int64_t>(system_request.request_rid_, his_->req_sys_dfd_.getOffset(EKORDDataID::eTxStamp));
        a_content->addData<uint16_t>(system_request.system_request_type_,
                                     his_->req_sys_dfd_.getOffset(EKORDDataID::eCTRCommandItem));

        auto crc = CRC::Calculate(a_content->getItemData(), a_content->getItemDataLength(), CRC::CRC_16_MODBUS());
        a_content->addData<uint16_t>(crc, his_->req_sys_dfd_.getOffset(EKORDDataID::eCRCValue));

        return 0;
    }
    case eServerRequest: {
        auto server_request = dynamic_cast<const RequestServer &>(a_request);
        a_content->setItemID(EKORDItemID::eRequestServer);

        a_content->addData<uint16_t>(a_seq, his_->req_server_dfd_.getOffset(EKORDDataID::eSequenceNumber));
        a_content->addData<int64_t>(server_request.request_rid_, his_->req_server_dfd_.getOffset(EKORDDataID::eTxStamp));

        a_content->addData<uint16_t>(server_request.command_,
                                     his_->req_server_dfd_.getOffset(EKORDDataID::eServerServiceCommand));
        a_content->addData<uint16_t>(server_request.service_id_,
                                     his_->req_server_dfd_.getOffset(EKORDDataID::eServerServiceId));

        // We should encode parameters, so we can send it via KORD, if they are set
        if (server_request.parameters_ != nullptr) {
            std::vector<uint8_t> enc_parameters = server_request.parameters_->dump();
            a_content->addData(enc_parameters, his_->req_server_dfd_.getOffset(EKORDDataID::eServerPayload));
        }

        auto crc = CRC::Calculate(a_content->getItemData(), a_content->getItemDataLength(), CRC::CRC_16_MODBUS());
        a_content->addData<uint16_t>(crc, his_->req_server_dfd_.getOffset(EKORDDataID::eCRCValue));
        return 0;
    }
    default: {
        return -2;
    }
    }
}

int KordCore::setStatusEchoContentItem(DefaultContentItem *a_content, const StatusFrameEcho &a_echo)
{
    if (!a_content)
        return -1;
    a_content->clear();

    a_content->setItemID(EKORDItemID::eStatusEchoResponse);
    a_content->addData<int64_t>(a_echo.tx_time_stamp_, his_->res_statusEcho_dfd_.getOffset(EKORDDataID::eTxStamp));
    a_content->addData<int64_t>(a_echo.rx_time_stamp_, his_->res_statusEcho_dfd_.getOffset(EKORDDataID::eRxStamp));

    std::uint32_t crc = CRC::Calculate(a_content->getItemData(), a_content->getItemDataLength(), CRC::CRC_16_MODBUS());
    a_content->addData<uint16_t>(crc, his_->res_statusEcho_dfd_.getOffset(EKORDDataID::eCRCValue));

    return 0;
}

unsigned int KordCore::sendFrame(const KORDFrame *a_frame) { return conn_container_->sendFrame(a_frame); }

timespec KordCore::ctlrcUpdateTS() const { return ctlrc_update_ts_; }

void KordCore::setCtlrcUpdateTS(timespec new_ts) { ctlrc_update_ts_ = new_ts; }

unsigned int KordCore::requestArmStatus()
{
    makeArmStatusRequest(&his_->tx_content_items_[0]);
    his_->content_builder_.clear();
    his_->content_builder_.addContentItem(his_->tx_content_items_[0]);
    his_->populateFrameFromContent(his_->eth_kord_proto_frm_, his_->content_builder_);
    return conn_container_->sendFrame(&his_->eth_kord_proto_frm_);
}

void KordCore::updateRecentCommandStatus(CommandStatuses &a_command_statuses) const
{
    auto last_command_status_token_id_ =
        his_->status_parser_.getData<uint64_t>(his_->status_dfd_.getOffset(EKORDDataID::eLastCommandToken));
    auto last_command_status_error_code_ =
        his_->status_parser_.getData<int8_t>(his_->status_dfd_.getOffset(EKORDDataID::eLastCommandErrorCode));

    CommandStatus last_command_status = CommandStatus();
    last_command_status.token = last_command_status_token_id_;
    last_command_status.error_code = last_command_status_error_code_;
    a_command_statuses.addStatus(last_command_status);
}

void KordCore::getRecentCommandStatus(CommandStatuses &a_command_statuses) const
{
    a_command_statuses = his_->sync_command_statuses_;
}

// Decoder of temperature, received in the form of uint16_t (a...b) = a...,b
double temperatureDecoder(const uint16_t &a_temp_encoded)
{
    int after_point = a_temp_encoded % 10;
    int before_point = a_temp_encoded / 10;
    return before_point + double(after_point) / 10;
}

std::array<double, 7> transformTemperaturesArray(const std::array<uint16_t, 7> &an_array)
{
    std::array<double, 7> tmp{};

    std::transform(an_array.begin(), an_array.end(), tmp.begin(), [](const uint16_t &c_temp) {
        return temperatureDecoder(c_temp);
    });

    return tmp;
}

void KordCore::getRecentArmStatus(RobotArmStatus &a_status) const { a_status = his_->sync_arm_status_; }

void KordCore::updateRecentArmStatus(RobotArmStatus &a_status) const
{
    // TBC This should be preallocated
    std::vector<uint8_t> buffer{};

    // clang-format off
    a_status.positions_ = his_->status_parser_.getData<std::array<double, 7>>(his_->status_dfd_.getOffset(EKORDDataID::eJConfigurationArm));
    a_status.torques_ = his_->status_parser_.getData<std::array<double, 7>>(his_->status_dfd_.getOffset(EKORDDataID::eJTorqueArm));
    a_status.speed_ = his_->status_parser_.getData<std::array<double, 7>>(his_->status_dfd_.getOffset(EKORDDataID::eJSpeedArm));
    a_status.accelerations_ = his_->status_parser_.getData<std::array<double, 7>>(his_->status_dfd_.getOffset(EKORDDataID::eJAccelerationArm));
    a_status.tcp_model_ = his_->status_parser_.getData<std::array<double, 6>>(his_->status_dfd_.getOffset(EKORDDataID::eFrmTCPPose));

    a_status.tcp_model_quaternion_ = utils::convertEulerToQuaternion(a_status.tcp_model_[0], a_status.tcp_model_[1], a_status.tcp_model_[2], a_status.tcp_model_[3], a_status.tcp_model_[4], a_status.tcp_model_[5]);

    auto j1 = his_->status_parser_.getData<std::array<double, 6>>(his_->status_dfd_.getOffset(EKORDDataID::eFrmJoint1Pose));
    auto j2 = his_->status_parser_.getData<std::array<double, 6>>(his_->status_dfd_.getOffset(EKORDDataID::eFrmJoint2Pose));
    auto j3 = his_->status_parser_.getData<std::array<double, 6>>(his_->status_dfd_.getOffset(EKORDDataID::eFrmJoint3Pose));
    auto j4 = his_->status_parser_.getData<std::array<double, 6>>(his_->status_dfd_.getOffset(EKORDDataID::eFrmJoint4Pose));
    auto j5 = his_->status_parser_.getData<std::array<double, 6>>(his_->status_dfd_.getOffset(EKORDDataID::eFrmJoint5Pose));
    auto j6 = his_->status_parser_.getData<std::array<double, 6>>(his_->status_dfd_.getOffset(EKORDDataID::eFrmJoint6Pose));
    auto j7 = his_->status_parser_.getData<std::array<double, 6>>(his_->status_dfd_.getOffset(EKORDDataID::eFrmJoint7Pose));

    a_status.joint_quaternion_[0] = utils::convertEulerToQuaternion(j1[0], j1[1], j1[2], j1[3], j1[4], j1[5]);
    a_status.joint_quaternion_[1] = utils::convertEulerToQuaternion(j2[0], j2[1], j2[2], j2[3], j2[4], j2[5]);
    a_status.joint_quaternion_[2] = utils::convertEulerToQuaternion(j3[0], j3[1], j3[2], j3[3], j3[4], j3[5]);
    a_status.joint_quaternion_[3] = utils::convertEulerToQuaternion(j4[0], j4[1], j4[2], j4[3], j4[4], j4[5]);
    a_status.joint_quaternion_[4] = utils::convertEulerToQuaternion(j5[0], j5[1], j5[2], j5[3], j5[4], j5[5]);
    a_status.joint_quaternion_[5] = utils::convertEulerToQuaternion(j6[0], j6[1], j6[2], j6[3], j6[4], j6[5]);
    a_status.joint_quaternion_[6] = utils::convertEulerToQuaternion(j7[0], j7[1], j7[2], j7[3], j7[4], j7[5]);

    // Joints Temperatures
    std::array<uint16_t, 7> coded_temperatures{};
    coded_temperatures = his_->status_parser_.getData<std::array<uint16_t, 7>>(his_->status_dfd_.getOffset(EKORDDataID::eJTemperatureBoard));

    a_status.jbs_board_temperatures_ = transformTemperaturesArray(coded_temperatures);

    coded_temperatures = his_->status_parser_.getData<std::array<uint16_t, 7>>(his_->status_dfd_.getOffset(EKORDDataID::eJTemperatureJointEncoder));
    a_status.jbs_joint_encoder_temperatures_ = transformTemperaturesArray(coded_temperatures);

    coded_temperatures = his_->status_parser_.getData<std::array<uint16_t, 7>>(his_->status_dfd_.getOffset(EKORDDataID::eJTemperatureRotorEncoder));
    a_status.jbs_rotor_encoder_temperatures_ = transformTemperaturesArray(coded_temperatures);

    // Joints Sensed Torques
    a_status.sens_torques_ = his_->status_parser_.getData<std::array<double, 7>>(his_->status_dfd_.getOffset(EKORDDataID::eJTorqueModel));

    // Joint Torques Deviation
    auto trq_dev_float = his_->status_parser_.getData<std::array<float, 7>>(his_->status_dfd_.getOffset(EKORDDataID::eJTorqueDeviation));
    std::transform(trq_dev_float.begin(), trq_dev_float.end(), a_status.torque_deviation_smooth_.begin(), [](float f) { return static_cast<double>(f); });
    
    // CPU State
    unsigned int cpu_state_id_ = his_->status_parser_.getData<uint8_t>(his_->status_dfd_.getOffset(EKORDDataID::eCPUStateId));
    a_status.cpu_state_[cpu_state_id_] = temperatureDecoder(his_->status_parser_.getData<uint16_t>(his_->status_dfd_.getOffset(EKORDDataID::eCPUStateVal)));

    // IOBoard Temperature
    a_status.iob_temperature_ = temperatureDecoder(his_->status_parser_.getData<uint16_t>(his_->status_dfd_.getOffset(EKORDDataID::eIOBCabinetTemperature)));

    // Frames
    unsigned int frame_id_ = his_->status_parser_.getData<uint8_t>(his_->status_dfd_.getOffset(EKORDDataID::eFrameId));
    unsigned int ref_frame_ = his_->status_parser_.getData<uint8_t>(his_->status_dfd_.getOffset(EKORDDataID::eFramePoseRef));
    a_status.frames_[frame_id_].pose_[ref_frame_] = his_->status_parser_.getData<std::array<double, 6>>(his_->status_dfd_.getOffset(EKORDDataID::eFramePose));

    // Loads
    unsigned int load_id_ = his_->status_parser_.getData<uint8_t>(his_->status_dfd_.getOffset(EKORDDataID::eLoadId));
    a_status.loads_[load_id_].cog = his_->status_parser_.getData<std::array<double, 3>>(his_->status_dfd_.getOffset(EKORDDataID::eLoadCoG));
    a_status.loads_[load_id_].mass = his_->status_parser_.getData<double>(his_->status_dfd_.getOffset(EKORDDataID::eLoadMass));
    a_status.loads_[load_id_].pose = his_->status_parser_.getData<std::array<double, 6>>(his_->status_dfd_.getOffset(EKORDDataID::eLoadPose));
    a_status.loads_[load_id_].inertia = his_->status_parser_.getData<std::array<double, 6>>(his_->status_dfd_.getOffset(EKORDDataID::eLoadInertia));

    a_status.cbun_stats_.fail_to_read_empty = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eFailToReadEmpty));
    a_status.cbun_stats_.fail_to_read_error = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eFailToReadError));

    // CBun stats
    a_status.cbun_stats_.cmd_jitter_window_max = his_->status_parser_.getData<int64_t>(his_->status_dfd_.getOffset(EKORDDataID::eMaxCmdJitterWindow));
    a_status.cbun_stats_.cmd_jitter_window_avg = his_->status_parser_.getData<int64_t>(his_->status_dfd_.getOffset(EKORDDataID::eAvgCmdJitterWindow));
    a_status.cbun_stats_.cmd_jitter_global_max = his_->status_parser_.getData<int64_t>(his_->status_dfd_.getOffset(EKORDDataID::eMaxCmdJitterGlobal));
    a_status.cbun_stats_.round_trip_window_max = his_->status_parser_.getData<int64_t>(his_->status_dfd_.getOffset(EKORDDataID::eMaxTickDelayWindow));
    a_status.cbun_stats_.round_trip_window_avg = his_->status_parser_.getData<int64_t>(his_->status_dfd_.getOffset(EKORDDataID::eAvgTickDelayWindow));
    a_status.cbun_stats_.round_trip_global_max = his_->status_parser_.getData<int64_t>(his_->status_dfd_.getOffset(EKORDDataID::eMaxTickDelayGlobal));
    a_status.cbun_stats_.cmd_lost_window_seq = his_->status_parser_.getData<int64_t>(his_->status_dfd_.getOffset(EKORDDataID::eCmdLostWindowSeq));
    a_status.cbun_stats_.cmd_lost_global_seq = his_->status_parser_.getData<int64_t>(his_->status_dfd_.getOffset(EKORDDataID::eCmdLostGlobalSeq));
    a_status.cbun_stats_.cmd_lost_window_timestmp = his_->status_parser_.getData<int64_t>(his_->status_dfd_.getOffset(EKORDDataID::eCmdLostWindowTimestamp));
    a_status.cbun_stats_.cmd_lost_global_timestmp = his_->status_parser_.getData<int64_t>(his_->status_dfd_.getOffset(EKORDDataID::eCmdLostGlobalTimestamp));
    a_status.cbun_stats_.system_jitter_window_max = his_->status_parser_.getData<int32_t>(his_->status_dfd_.getOffset(EKORDDataID::eMaxSysJitterWindow));
    a_status.cbun_stats_.system_jitter_window_avg = his_->status_parser_.getData<int32_t>(his_->status_dfd_.getOffset(EKORDDataID::eAvgSysJitterWindow));
    a_status.cbun_stats_.system_jitter_global_max = his_->status_parser_.getData<int32_t>(his_->status_dfd_.getOffset(EKORDDataID::eMaxSysJitterGlobal));

    // Controller
    a_status.rc_motion_flags_ = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eRCMotionFlags));
    a_status.rc_safety_flags_ = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eRCSafetyFlag));
    a_status.rc_safety_mode_ = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eRCSafetyMode));
    a_status.rc_master_speed_ = his_->status_parser_.getData<double>(his_->status_dfd_.getOffset(EKORDDataID::eRCMasterSpeed));
    a_status.rc_hw_flags_ = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eRCHWFlags));
    a_status.rc_button_flags_ = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eRCButtonFlags));
    a_status.rc_system_alarm_state_ = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eRCSystemAlarmState));

    a_status.max_frames_in_tick = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eMaxFramesInTick));
    a_status.faulty_frames_start = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eFaultyFramesStart));

    a_status.rc_digital_output_ = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eIODigitalOutput));
    a_status.rc_digital_input_ = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eIODigitalInput));
    a_status.rc_safe_digital_config = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eIODigitalSafetyMask));
    // eIOAnalogInput
    // eIOAnalogOutput

    a_status.latest_request_system_.request_rid_ = his_->status_parser_.getData<int64_t>(his_->status_dfd_.getOffset(EKORDDataID::eCTRCommandTS));
    a_status.latest_request_system_.system_request_type_ = static_cast<EControlCommandItems>(his_->status_parser_.getData<uint16_t>(his_->status_dfd_.getOffset(EKORDDataID::eCTRCommandItem)));
    a_status.latest_request_system_.request_status_ = static_cast<EControlCommandStatus>(his_->status_parser_.getData<uint16_t>(his_->status_dfd_.getOffset(EKORDDataID::eCTRCommandStatus)));
    a_status.latest_request_system_.error_code_ = his_->status_parser_.getData<uint32_t>(his_->status_dfd_.getOffset(EKORDDataID::eCTRCommandErrorCode));

    // TBC - fill the array/vector
    // TBC - Do check the return value
    his_->status_parser_.getData(buffer, his_->status_dfd_.getOffset(EKORDDataID::eEventsArray));
    // clang-format on

    // If the buffer is not empty there should be some events present
    // Do the conversion and unpack the events contained.
    // This loop should is TBC, an example fo fixed array

    for (SystemEvent &event : a_status.system_events_) {
        event.reset();
    }

    if (!buffer.empty() && buffer.size() >= sizeof(SystemEvent)) {
        int offset = 0;

        for (SystemEvent &event : a_status.system_events_) {
            if (buffer.size() - offset < sizeof(SystemEvent)) {
                break;
            }

            // Create a temporary vector representing the current slice of the buffer
            std::vector<uint8_t> current_slice(buffer.begin() + offset, buffer.begin() + offset + sizeof(SystemEvent));

            // Initialize the event from the current slice of the buffer
            if (!event.initFromByteArray(current_slice)) {
                break;
            }
            // Move the offset forward by the size of SystemEvent
            offset += sizeof(SystemEvent);
        }

        // Erase only the part of the buffer that has been processed
        buffer.erase(buffer.begin(), buffer.begin() + offset);
    }
}

unsigned int KordCore::sendCommand(RobotArmCommand a_cmd)
{
    switch (a_cmd.command_type) {
    case RobotArmCommand::EType::eFW: {
        token_t not_used_token_;
        makeCommandFirmware(&his_->tx_content_items_[0], a_cmd.fw_cmds_, a_cmd.seq_, not_used_token_);
        break;
    }
    case RobotArmCommand::EType::eMOVE: {
        makeCommandMoveJ(&his_->tx_content_items_[0], a_cmd);
        break;
    }
    case RobotArmCommand::EType::eMOVEManifold: {
        makeCommandMoveManifold(&his_->tx_content_items_[0], a_cmd);
        break;
    }
    case RobotArmCommand::EType::eDJC: {
        makeCommandMoveD(&his_->tx_content_items_[0],
                         a_cmd.positions_,
                         a_cmd.speed_,
                         a_cmd.accelerations_,
                         a_cmd.torque_,
                         a_cmd.seq_);
        break;
    }
    case RobotArmCommand::EType::eDTC: {
        makeCommandMoveDirectTorque(&his_->tx_content_items_[0], a_cmd.torque_, a_cmd.seq_);
        break;
    }
    default: {
        // error
        return 0;
    }
    }

    his_->content_builder_.clear();
    his_->content_builder_.addContentItem(his_->tx_content_items_[0]);
    his_->populateFrameFromContent(his_->eth_kord_proto_frm_, his_->content_builder_);

    return conn_container_->sendFrame(&his_->eth_kord_proto_frm_);
}

unsigned int KordCore::sendCommand(RobotArmCommand a_cmd, int64_t &a_returned_token)
{
    switch (a_cmd.command_type) {
    case RobotArmCommand::EType::eFW: {
        makeCommandFirmware(&his_->tx_content_items_[0], a_cmd.fw_cmds_, a_cmd.seq_, a_returned_token);
        break;
    }
    case RobotArmCommand::EType::eMOVE: {
        makeCommandMoveJ(&his_->tx_content_items_[0], a_cmd);
        break;
    }
    case RobotArmCommand::EType::eMOVEManifold: {
        makeCommandMoveManifold(&his_->tx_content_items_[0], a_cmd);
        break;
    }
    case RobotArmCommand::EType::eDJC: {
        makeCommandMoveD(&his_->tx_content_items_[0],
                         a_cmd.positions_,
                         a_cmd.speed_,
                         a_cmd.accelerations_,
                         a_cmd.torque_,
                         a_cmd.seq_);
        break;
    }
    default: {
        // error
        return 0;
    }
    }

    his_->content_builder_.clear();
    his_->content_builder_.addContentItem(his_->tx_content_items_[0]);
    his_->populateFrameFromContent(his_->eth_kord_proto_frm_, his_->content_builder_);

    return conn_container_->sendFrame(&his_->eth_kord_proto_frm_);
}

unsigned int KordCore::sendCommand(RobotFrameCommand a_cmd)
{
    switch (a_cmd.command_type) {
    case RobotFrameCommand::MOVE_POSE: {
        makeCommandMoveL(&his_->tx_content_items_[0], a_cmd);
        break;
    }
    case RobotFrameCommand::MOVE_POSE_QUAT: {
        makeCommandMoveLQuat(&his_->tx_content_items_[0], a_cmd);
        break;
    }
    case RobotFrameCommand::MOVE_VELOCITY: {
        makeCommandMoveVelocityL(&his_->tx_content_items_[0],
                                 a_cmd.tcp_target_velocity_,
                                 a_cmd.seq_,
                                 a_cmd.sync_value_,
                                 a_cmd.period_,
                                 a_cmd.timeout_);
        break;
    }
    case RobotFrameCommand::MOVE_POSE_DYN: {
        makeCommandMoveDynL(&his_->tx_content_items_[0], a_cmd.tcp_target_, a_cmd.seq_);
        break;
    }
    default: {
        break;
    }
    }

    his_->content_builder_.clear();
    his_->content_builder_.addContentItem(his_->tx_content_items_[0]);
    his_->populateFrameFromContent(his_->eth_kord_proto_frm_, his_->content_builder_);

    return conn_container_->sendFrame(&his_->eth_kord_proto_frm_);
}

unsigned int KordCore::sendCommand(RobotSetupCommand a_cmd, int64_t &a_returned_token)
{
    switch (a_cmd.command_type) {
    case RobotSetupCommand::EType::SET_FRAME: {
        makeCommandSetFrame(&his_->tx_content_items_[0], a_cmd, a_returned_token);
        break;
    }

    case RobotSetupCommand::SET_LOAD: {
        makeCommandSetLoad(&his_->tx_content_items_[0], a_cmd, a_returned_token);
        break;
    }

    case RobotSetupCommand::CLEAN_ALARM: {
        makeCommandCleanAlarm(&his_->tx_content_items_[0], a_cmd, a_returned_token);
        break;
    }
    default: {
        break;
    }
    }

    his_->content_builder_.clear();
    his_->content_builder_.addContentItem(his_->tx_content_items_[0]);
    his_->populateFrameFromContent(his_->eth_kord_proto_frm_, his_->content_builder_);

    return conn_container_->sendFrame(&his_->eth_kord_proto_frm_);
}

unsigned int KordCore::sendCommand(const Request &a_request)
{
    setRequestContentItem(&his_->tx_content_items_[0], a_request, his_->req_sys_seq_num_);
    his_->content_builder_.clear();
    his_->content_builder_.addContentItem(his_->tx_content_items_[0]);
    his_->populateFrameFromContent(his_->eth_kord_proto_frm_, his_->content_builder_);
    return conn_container_->sendFrame(&his_->eth_kord_proto_frm_);
}

unsigned int KordCore::sendCommand(const Request &a_request, token_t &command_token_id_)
{
    command_token_id_ = a_request.request_rid_;

    // Determine the appropriate container and prepare the frame.
    std::shared_ptr<ConnectionInterface> container = determineContainerAndPrepareFrame(a_request);

    // Send the frame using the selected container.
    return sendPreparedFrame(container, a_request);
}

std::shared_ptr<ConnectionInterface> KordCore::determineContainerAndPrepareFrame(const Request &a_request)
{
    if (isTCPRequest(a_request)) {
        prepareTCPFrame(a_request);
        return conn_container_requests_;
    }

    prepareUDPFrame(a_request);
    return conn_container_;
}

bool KordCore::isTCPRequest(const Request &a_request) const
{
    return (a_request.request_type_ == EKORDItemID::eRequestSystem &&
            (a_request.system_request_type_ == eGetVersion || a_request.system_request_type_ == eGetSafetyZones ||
             a_request.system_request_type_ == eServerEnableCommunication ||
             a_request.system_request_type_ == eServerDisableCommunication ||
             a_request.system_request_type_ == eGetRobotInfo)) ||
           a_request.request_type_ == EKORDItemID::eRequestServer;
}

void KordCore::prepareTCPFrame(const Request &a_request)
{
    setRequestContentItem(&his_->tx_content_items_tcp_[0], a_request, his_->req_sys_seq_num_);
    his_->content_builder_tcp_.clear();
    his_->content_builder_tcp_.addContentItem(his_->tx_content_items_tcp_[0]);
    his_->populateFrameFromContent(his_->tcp_kord_proto_frm_, his_->content_builder_tcp_);
}

void KordCore::prepareUDPFrame(const Request &a_request)
{
    setRequestContentItem(&his_->tx_content_items_[0], a_request, his_->req_sys_seq_num_);
    his_->content_builder_.clear();
    his_->content_builder_.addContentItem(his_->tx_content_items_[0]);
    his_->populateFrameFromContent(his_->eth_kord_proto_frm_, his_->content_builder_);
}

unsigned int KordCore::sendPreparedFrame(std::shared_ptr<ConnectionInterface> &container, const Request &a_request)
{
    if (isTCPRequest(a_request)) {
        return container->sendFrame(&his_->tcp_kord_proto_frm_);
    }

    return container->sendFrame(&his_->eth_kord_proto_frm_);
}

unsigned int KordCore::sendCommand(const KordCore::StatusFrameEcho &a_response)
{
    setStatusEchoContentItem(&his_->tx_content_items_[0], a_response);
    his_->content_builder_.clear();
    his_->content_builder_.addContentItem(his_->tx_content_items_[0]);
    his_->populateFrameFromContent(his_->eth_kord_proto_frm_, his_->content_builder_);
    return conn_container_->sendFrame(&his_->eth_kord_proto_frm_);
}
