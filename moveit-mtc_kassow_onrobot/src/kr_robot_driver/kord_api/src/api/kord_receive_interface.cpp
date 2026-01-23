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

#include <kord/api/kord_receive_interface.h>
#include <kord/utils/logger.h>

#include <kord/protocol/RequestResponses/Response.h>
#include <kord/protocol/RequestResponses/ResponseGetRobotInfo.h>
#include <kord/protocol/RequestResponses/ResponseGetSafetyZones.h>
#include <kord/protocol/RequestResponses/ResponseGetVersion.h>
#include <kord/protocol/RequestResponses/ResponseServer.h>
#include <kord/protocol/RequestResponses/ResponseSoftwareUpdate.h>

using namespace kr2::kord;

struct ReceiverInterface::Internals {
    enum { MOTION_FLAG_STANDBY = 1 };
    enum { MOTION_FLAG_TRACKING = 2 };
    enum { MOTION_FLAG_TERMINATED = 4 };
    enum { MOTION_FLAG_HALT = 8 };
    enum { MOTION_FLAG_SYNC = 16 };
    enum { MOTION_FLAG_SUSPENDED = 32 };
    enum { MOTION_FLAG_OFFLINE = 64 };
    enum { MOTION_FLAG_INIT = 128 };
    enum { MOTION_FLAG_REINIT = 256 };
    enum { MOTION_FLAG_BACKDRIVE = 512 };
    enum { MOTION_FLAG_PAUSED = 1024 };
    enum { MOTION_FLAG_MAINTENANCE = 2048 };
    enum { MOTION_FLAG_VELOCITYCTL = 4096 };
    enum { MOTION_FLAG_ARTOACTIVE = 8192 };

    enum { SAFETY_FLAG_UPDATE = 1 };
    enum { SAFETY_FLAG_ESTOP = 2 };
    enum { SAFETY_FLAG_PSTOP = 4 };
    enum { SAFETY_FLAG_SSTOP = 8 };
    enum { SAFETY_FLAG_USER_CONF_REQ = 16 };

    enum { SAFETY_MODE_NORMAL = 1 };
    enum { SAFETY_MODE_REDUCED = 2 };
    enum { SAFETY_MODE_SAFE = 3 };

    enum { BUTTONS_FLAG_ESTOP = 0x01 };
    enum { BUTTONS_FLAG_PSTOP = 0x02 };
    enum { BUTTONS_FLAG_TOGGLE = 0x04 };
    enum { BUTTONS_FLAG_BACKDRIVE = 0x08 };
    enum { BUTTONS_FLAG_TEACH = 0x10 };

    enum { HW_STATUS_ERROR_BITS_SET = 0x0200 }; // TODO: Duplicated value, but with status

    enum { HW_FLAG_RUID_MISMATCH = 0x0004 };
    enum { HW_FLAG_HARD_FAULT = 0x0008 };
    enum { HW_FLAG_DISCOVERY_FAILED = 0x0100 };
    enum { HW_FLAG_ERROR_BITS_SET = 0x0200 };
    enum { HW_FLAG_DEVICE_DISABLED = 0x0400 };
    enum { HW_FLAG_LOW_VOLTAGE = 0x0800 };
    enum { HW_FLAG_IOB_INIT_TIMEOUT = 0x1000 };
    enum { HW_FLAG_SYNC_INIT_TIMEOUT = 0x2000 };
    enum { HW_FLAG_IOB_ESTOP_STALL = 0x4000 };
    enum { HW_FLAG_IOB_PSTOP_STALL = 0x8000 };

    enum { CAT_SAFETY_EVENT = 0x01 };
    enum { CAT_SOFT_STOP_EVENT = 0x02 };
    enum { CAT_HW_STAT = 0x03 };
    enum { CAT_CBUN_EVENT = 0x04 };

    enum { CNTXT_ESTOP = 0x01 };
    enum { CNTXT_PSTOP = 0x02 };
    enum { CNTXT_SSTOP = 0x04 };
    enum { CNTXT_SYSERR = 0x08 };

    KordCore::RobotArmStatus rs_;
    std::shared_ptr<KordCore> kord_;

    KordCore::CommandStatuses command_statuses_;
};

ReceiverInterface::ReceiverInterface(std::shared_ptr<KordCore> a_kord) : his_(new Internals) { his_->kord_ = a_kord; }

ReceiverInterface::~ReceiverInterface() { his_->kord_->disconnect(); }

bool ReceiverInterface::sync()
{
    std::scoped_lock lock(comm_mutex_);
    his_->kord_->syncRC();

    his_->kord_->getRecentArmStatus(his_->rs_);
    his_->kord_->getRecentCommandStatus(his_->command_statuses_);

    return true;
}

bool ReceiverInterface::sendStatusEcho(int64_t a_time_stamp)
{
    kord::KordCore::StatusFrameEcho echo_cmd{};

    echo_cmd.tx_time_stamp_ = a_time_stamp;
    echo_cmd.rx_time_stamp_ = std::chrono::system_clock::now().time_since_epoch().count();

    unsigned int n = his_->kord_->sendCommand(echo_cmd);

    if (n <= 0)
        return false;

    return true;
}

bool ReceiverInterface::fetchStatus()
{
    if (!his_->kord_->waitSync(std::chrono::milliseconds(10), kord::F_NONE)) {
        sendStatusEcho(his_->kord_->getTxStamp()); // anyway sending old tx back to see the error on the CBun side
        return false;
    }
    comm_mutex_.lock();
    fetchData();
    comm_mutex_.unlock();

    sendStatusEcho(his_->kord_->getTxStamp());
    return true;
}

void ReceiverInterface::fetchData()
{
    his_->kord_->getRecentArmStatus(his_->rs_);
    his_->kord_->getRecentCommandStatus(his_->command_statuses_);
}

std::array<double, 7UL> ReceiverInterface::getJoint(EJointValue a_jv)
{
    std::scoped_lock lock(comm_mutex_);
    switch (a_jv) {
    case EJointValue::S_ACTUAL_Q:
        return his_->rs_.positions_;
    case EJointValue::S_ACTUAL_QD:
        return his_->rs_.speed_;
    case EJointValue::S_ACTUAL_QDD:
        return his_->rs_.accelerations_;
    case EJointValue::S_ACTUAL_TRQ:
        return his_->rs_.torques_;
    case EJointValue::S_TEMP_BOARD:
        return his_->rs_.jbs_board_temperatures_;
    case EJointValue::S_TEMP_JOINT_ENCODER:
        return his_->rs_.jbs_joint_encoder_temperatures_;
    case EJointValue::S_TEMP_ROTOR_ENCODER:
        return his_->rs_.jbs_rotor_encoder_temperatures_;
    case EJointValue::S_SENSED_TRQ:
        return his_->rs_.sens_torques_;
    case EJointValue::S_TRQDEV_SMOOTH:
        return his_->rs_.torque_deviation_smooth_;
    default:
        break;
    }

    return {};
}

std::vector<std::variant<double, int>> ReceiverInterface::getFrame(EFrameID a_frame_id, EFrameValue a_frame_value)
{
    std::scoped_lock lock(comm_mutex_);

    std::vector<std::variant<double, int>> a_output;

    if ((his_->rs_.frames_.find(a_frame_id) == his_->rs_.frames_.end()) ||
        (his_->rs_.frames_[a_frame_id].pose_.find(a_frame_value) == his_->rs_.frames_[a_frame_id].pose_.end())) {
        return a_output;
    }

    for (auto i : his_->rs_.frames_[a_frame_id].pose_[a_frame_value]) {
        a_output.emplace_back(i);
    }

    return a_output;
}

std::vector<std::variant<double, int>> ReceiverInterface::getLoad(ELoadID a_load_id, ELoadValue a_load_value)
{
    std::scoped_lock lock(comm_mutex_);

    std::vector<std::variant<double, int>> a_output;

    if ((his_->rs_.loads_.find(a_load_id) == his_->rs_.loads_.end())) {
        return a_output;
    }

    switch (a_load_value) {
    case ELoadValue::COG_VAL: {
        for (uint32_t i = 0; i < his_->rs_.loads_[a_load_id].cog.size(); ++i) {
            a_output.emplace_back(his_->rs_.loads_[a_load_id].cog[i]);
        }
        break;
    }
    case ELoadValue::INERTIA_VAL: {
        for (uint32_t i = 0; i < his_->rs_.loads_[a_load_id].inertia.size(); ++i) {
            a_output.emplace_back(his_->rs_.loads_[a_load_id].inertia[i]);
        }
        break;
    }
    case ELoadValue::POSE_VAL: {
        for (uint32_t i = 0; i < his_->rs_.loads_[a_load_id].pose.size(); ++i) {
            a_output.emplace_back(his_->rs_.loads_[a_load_id].pose[i]);
        }
        break;
    }
    case ELoadValue::MASS_VAL: {
        a_output.emplace_back(his_->rs_.loads_[a_load_id].mass);
        break;
    }
    }

    return a_output;
}

std::array<double, 6UL> ReceiverInterface::getTCP() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.tcp_model_;
}

std::array<double, 7ul> ReceiverInterface::getTCPWithQuaternion() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.tcp_model_quaternion_;
}

std::array<double, 7ul> ReceiverInterface::getJointPoseWithQuaternion(int joint_num) const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.joint_quaternion_[joint_num - 1];
}

unsigned int ReceiverInterface::getButtonFlags() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.rc_button_flags_;
}

unsigned int ReceiverInterface::getHWFlags() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.rc_hw_flags_;
}

double ReceiverInterface::getCPUState(unsigned int a_state)
{
    std::scoped_lock lock(comm_mutex_);

    double a_output = 0.0;

    if (his_->rs_.cpu_state_.find(a_state) == his_->rs_.cpu_state_.end()) {
        return a_output;
    }

    a_output = his_->rs_.cpu_state_[a_state];
    return a_output;
}

int8_t ReceiverInterface::getCommandStatus(token_t a_token)
{
    std::scoped_lock lock(comm_mutex_);

    KordCore::CommandStatus status;
    if (his_->command_statuses_.getPresentStatus(a_token, status)) {
        return status.error_code;
    }

    return -1; // not found
}

uint64_t ReceiverInterface::getLatestCommandToken() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->command_statuses_.getLatestStatusToken();
}

unsigned int ReceiverInterface::getRobotSafetyFlags() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.rc_safety_flags_;
}

double ReceiverInterface::getMasterSpeed() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.rc_master_speed_;
}

double ReceiverInterface::getIOBoardTemperature() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.iob_temperature_;
}

int ReceiverInterface::getSafetyMode() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.rc_safety_mode_;
}

unsigned int ReceiverInterface::getMotionFlags() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.rc_motion_flags_;
}

uint32_t ReceiverInterface::systemAlarmState() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.rc_system_alarm_state_;
}

int64_t ReceiverInterface::getStatistics(EStatsValue stats_type)
{
    std::scoped_lock lock(comm_mutex_);

    switch (stats_type) {
    case EStatsValue::FAIL_TO_READ_EMPTY:
        return his_->rs_.cbun_stats_.fail_to_read_empty;
    case EStatsValue::FAIL_TO_READ_ERROR:
        return his_->rs_.cbun_stats_.fail_to_read_error;
    case EStatsValue::CMD_JITTER_MAX_LOCAL:
        return his_->rs_.cbun_stats_.cmd_jitter_window_max;
    case EStatsValue::CMD_JITTER_AVG_LOCAL:
        return his_->rs_.cbun_stats_.cmd_jitter_window_avg;
    case EStatsValue::CMD_JITTER_MAX_GLOBAL:
        return his_->rs_.cbun_stats_.cmd_jitter_global_max;
    case EStatsValue::ROUND_TRIP_TIME_MAX_LOCAL:
        return his_->rs_.cbun_stats_.round_trip_window_max;
    case EStatsValue::ROUND_TRIP_TIME_AVG_LOCAL:
        return his_->rs_.cbun_stats_.round_trip_window_avg;
    case EStatsValue::ROUND_TRIP_TIME_MAX_GLOBAL:
        return his_->rs_.cbun_stats_.round_trip_global_max;
    case EStatsValue::CMD_LOST_COUNTER_LOCAL:
        return his_->rs_.cbun_stats_.cmd_lost_window_seq;
    case EStatsValue::CMD_LOST_COUNTER_GLOBAL:
        return his_->rs_.cbun_stats_.cmd_lost_global_seq;
    case EStatsValue::CMD_LOST_COUNTER_LOCAL_TIMESTAMP:
        return his_->rs_.cbun_stats_.cmd_lost_window_timestmp;
    case EStatsValue::CMD_LOST_COUNTER_GLOBAL_TIMESTAMP:
        return his_->rs_.cbun_stats_.cmd_lost_global_timestmp;
    case EStatsValue::SYS_JITTER_MAX_LOCAL:
        return his_->rs_.cbun_stats_.system_jitter_window_max;
    case EStatsValue::SYS_JITTER_AVG_LOCAL:
        return his_->rs_.cbun_stats_.system_jitter_window_avg;
    case EStatsValue::SYS_JITTER_MAX_GLOBAL:
        return his_->rs_.cbun_stats_.system_jitter_global_max;
    default:
        // If not found, return max value
        return INT64_MAX;
    }
}

CBunReceivedStatistics ReceiverInterface::getStatisticsStructure() const { return his_->rs_.cbun_stats_; }

size_t ReceiverInterface::getMaxFramesInTick() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.max_frames_in_tick;
}

int64_t ReceiverInterface::getFaultyTickFrame() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.faulty_frames_start;
}

int64_t ReceiverInterface::getDigitalOutput() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.rc_digital_output_;
}

uint32_t ReceiverInterface::getSafeDigitalOutputConfig() const
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.rc_safe_digital_config;
}

int64_t ReceiverInterface::getDigitalInput()
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.rc_digital_input_;
}

std::string ReceiverInterface::getFormattedInputBits()
{
    int64_t d_input = getDigitalInput();

    std::string in = std::bitset<16>(d_input).to_string();
    std::string out = "Digital Inputs  [16|DI| 1]: ";
    for (uint32_t i = 0; i < in.size(); i++) {
        if (i % 4 == 0 && i != 0)
            out += " ";
        out += in[i];
    }
    return out;
}

std::string ReceiverInterface::getFormattedOutputBits() const
{
    int64_t d_output = getDigitalOutput();

    std::string in = std::bitset<24>(d_output).to_string();
    std::string out = "Digital Outputs [PSU: 2 1] [4 |TB| 1] [8 |B| 1] [4 |R| 1]: ";
    for (uint32_t i = 0; i < in.size(); i++) {
        if (i == 0 || i == 1 || i == 8 || i == 9 || i == 10 || i == 11)
            continue; // ignore some outputs
        if (i % 4 == 0)
            out += " ";
        out += in[i];
    }
    return out;
}

Request ReceiverInterface::getLatestRequest()
{
    std::scoped_lock lock(comm_mutex_);
    return his_->rs_.latest_request_system_;
}

std::vector<protocol::SystemEvent> ReceiverInterface::getSystemEvents()
{
    std::scoped_lock lock(comm_mutex_);
    std::vector<protocol::SystemEvent> ret;

    std::copy_if(his_->rs_.system_events_.begin(),
                 his_->rs_.system_events_.end(),
                 std::back_inserter(ret),
                 [](auto &el) { return el.is_valid(); });

    return ret;
}

void ReceiverInterface::clearSystemEventsBuffer()
{
    std::scoped_lock lock(comm_mutex_);
    for (auto &event : his_->rs_.system_events_) {
        event.reset();
    }
}

bool ReceiverInterface::hasResponse(token_t token) const
{
    auto responses = his_->kord_->getResponses();
    return std::any_of(responses.begin(), responses.end(), [token](auto &response) {
        return response.getToken() == token;
    });
}

template <typename T> T ReceiverInterface::getResponse(token_t token)
{
    auto responses = his_->kord_->getResponses();
    auto f = std::find_if(responses.begin(), responses.end(), [token](auto &response) {
        return response.getToken() == token;
    });

    if (f == responses.end())
        throw std::runtime_error("Response not found");

    // delete the response from the list
    bool success = his_->kord_->eraseResponse(f->getToken());
    if (!success)
        KORD_LOG_ERROR("Failed to erase response");

    return T(f->getRawData());
}

template protocol::Response ReceiverInterface::getResponse(token_t token);
template protocol::GetVersionResponse ReceiverInterface::getResponse(token_t token);
template protocol::SoftwareUpdateResponse ReceiverInterface::getResponse(token_t token);
template protocol::ServerResponse ReceiverInterface::getResponse(token_t token);
template protocol::GetSafetyZonesResponse ReceiverInterface::getResponse(token_t token);
template protocol::GetRobotInfoResponse ReceiverInterface::getResponse(token_t token);
