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

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/utils/logger.h>
#include <kord/utils/timex.h>

#include <exception>
#include <fstream>
#include <iostream>

using namespace kr2::kord;
using namespace kr2::kord::protocol;

struct ControlInterface::Internals {
    std::shared_ptr<KordCore> kord_;
    unsigned short sequence_number_;
};

ControlInterface::ControlInterface(std::shared_ptr<kord::KordCore> kord) : his_(new Internals)
{
    his_->kord_ = kord;
    his_->sequence_number_ = 0;
}

ControlInterface::~ControlInterface() { his_->kord_->disconnect(); }

bool ControlInterface::moveJ(const std::array<double, 7UL> &a_q,
                             const TrackingType &a_tt,
                             const double &a_tt_value,
                             const BlendType &a_bt,
                             const double &a_bt_value,
                             const OverlayType &a_ot,
                             const double &a_sync_time)
{
    if (a_q.size() != 7)
        return false;

    kord::KordCore::RobotArmCommand arm_cmd;
    arm_cmd.command_type = KordCore::RobotArmCommand::EType::eMOVE;
    arm_cmd.positions_ = a_q;
    arm_cmd.tt_ = a_tt;
    arm_cmd.tt_value_ = a_tt_value;
    arm_cmd.bt_ = a_bt;
    arm_cmd.bt_value_ = a_bt_value;
    arm_cmd.ot_ = a_ot;
    arm_cmd.sync_time_ = a_sync_time;
    arm_cmd.seq_ = his_->sequence_number_;
    unsigned int n = his_->kord_->sendCommand(arm_cmd);
    his_->sequence_number_++;

    if (n <= 0)
        return false;

    return true;
}

bool ControlInterface::moveL(const std::array<double, 6UL> &a_p,
                             const TrackingType &a_tt,
                             const double &a_tt_value,
                             const BlendType &a_bt,
                             const double &a_bt_value,
                             const OverlayType &a_ot,
                             const double &sync_time)
{
    if (a_p.size() != 6)
        return false;

    kord::KordCore::RobotFrameCommand frm_cmd{};
    frm_cmd.asPoseControl()
        .withTargetPose(a_p)
        .withTrackingType(a_tt)
        .withTrackingValue(a_tt_value)
        .withBlendType(a_bt)
        .withBlendValue(a_bt_value)
        .withOverlayType(a_ot)
        .withSyncTime(sync_time)
        .setSequenceNumber(his_->sequence_number_);

    unsigned int n = his_->kord_->sendCommand(frm_cmd);
    his_->sequence_number_++;

    if (n <= 0)
        return false;

    return true;
}

bool ControlInterface::moveL(const std::array<double, 7UL> &pose_quat,
                             const TrackingType &tt,
                             const double &tt_value,
                             const BlendType &bt,
                             const double &bt_value,
                             const OverlayType &ot,
                             const double &sync_time)
{
    kord::KordCore::RobotFrameCommand frm_cmd{};

    frm_cmd.asPoseQuatControl()
        .withTargetPoseQuat(pose_quat)
        .withTrackingType(tt)
        .withTrackingValue(tt_value)
        .withBlendType(bt)
        .withBlendValue(bt_value)
        .withOverlayType(ot)
        .withSyncTime(sync_time)
        .setSequenceNumber(his_->sequence_number_);

    unsigned int n = his_->kord_->sendCommand(frm_cmd);
    his_->sequence_number_++;

    if (n <= 0)
        return false;

    return true;
}

bool ControlInterface::moveV(const std::array<double, 6UL> &v,
                             const long sync_value,
                             const double period,
                             const double timeout)
{
    if (v.size() != 6) {
        return false;
    }

    kord::KordCore::RobotFrameCommand frm_cmd{};

    frm_cmd.asVelocityControl()
        .withTargetVelocity(v)
        .withSyncValue(sync_value)
        .withPeriod(period)
        .withTimeout(timeout)
        .setSequenceNumber(his_->sequence_number_);

    unsigned int n = his_->kord_->sendCommand(frm_cmd);
    his_->sequence_number_++;

    if (n <= 0)
        return false;

    return true;
}

bool ControlInterface::moveManifold(double manifold_joint_speed)
{
    KordCore::RobotArmCommand arm_cmd;
    arm_cmd.command_type = KordCore::RobotArmCommand::eMOVEManifold;
    arm_cmd.manifold_joint_speed_ = manifold_joint_speed;

    arm_cmd.seq_ = his_->sequence_number_;
    unsigned int n = his_->kord_->sendCommand(arm_cmd);
    his_->sequence_number_++;

    if (n <= 0)
        return false;
    return true;
}

bool ControlInterface::setFrame(EFrameID frame_id, std::array<double, 6> pose, EFrameValue ref_frame)
{
    token_t not_used_token_;
    return setFrame(frame_id, pose, ref_frame, not_used_token_);
}

bool ControlInterface::setFrame(EFrameID frame_id, std::array<double, 6> pose, EFrameValue ref_frame, token_t &token)
{

    kord::KordCore::RobotSetupCommand set_cmd{};

    set_cmd.command_type = KordCore::RobotSetupCommand::EType::SET_FRAME;
    set_cmd.pose_ = pose;
    set_cmd.frame_id_ = frame_id;
    set_cmd.ref_frame_ = ref_frame;

    set_cmd.seq_ = his_->sequence_number_;
    int64_t command_token_id_;
    unsigned int n = his_->kord_->sendCommand(set_cmd, command_token_id_);
    his_->sequence_number_++;

    if (n <= 0)
        return false;

    token = command_token_id_;
    return true;
}

bool ControlInterface::setLoad(ELoadID load_id, double mass, std::array<double, 3> cog, std::array<double, 6> inertia)
{
    token_t not_used_token_;
    return setLoad(load_id, mass, cog, inertia, not_used_token_);
}

bool ControlInterface::setLoad(
    ELoadID load_id, double mass, std::array<double, 3> cog, std::array<double, 6> inertia, token_t &token)
{

    kord::KordCore::RobotSetupCommand set_cmd{};

    set_cmd.command_type = KordCore::RobotSetupCommand::EType::SET_LOAD;
    set_cmd.load_id_ = load_id;
    set_cmd.load_mass_ = mass;
    set_cmd.load_cog_ = cog;
    set_cmd.load_inertia_ = inertia;

    set_cmd.seq_ = his_->sequence_number_;
    int64_t command_token_id_;
    unsigned int n = his_->kord_->sendCommand(set_cmd, command_token_id_);
    his_->sequence_number_++;

    if (n <= 0)
        return false;

    token = command_token_id_;
    return true;
}

bool ControlInterface::directJControl(const std::array<double, 7UL> &q,
                                      const std::array<double, 7UL> &qd,
                                      const std::array<double, 7UL> &qdd)
{
    if (q.size() != 7) {
        return false;
    }
    if (qd.size() != 7) {
        return false;
    }
    if (qdd.size() != 7) {
        return false;
    }

    kord::KordCore::RobotArmCommand arm_cmd;
    arm_cmd.command_type = KordCore::RobotArmCommand::EType::eDJC;

    arm_cmd.positions_ = q;
    arm_cmd.speed_ = qd;
    arm_cmd.accelerations_ = qdd;
    arm_cmd.torque_ = {0., 0., 0., 0., 0., 0., 0.};

    arm_cmd.seq_ = his_->sequence_number_;
    unsigned int n = his_->kord_->sendCommand(arm_cmd);
    his_->sequence_number_++;

    return n > 0;
}

bool ControlInterface::directJControl(const std::array<double, 7UL> &q,
                                      const std::array<double, 7UL> &qd,
                                      const std::array<double, 7UL> &qdd,
                                      const std::array<double, 7UL> &torque)
{
    if (q.size() != 7) {
        return false;
    }
    if (qd.size() != 7) {
        return false;
    }
    if (qdd.size() != 7) {
        return false;
    }
    if (torque.size() != 7) {
        return false;
    }

    kord::KordCore::RobotArmCommand arm_cmd;
    arm_cmd.command_type = KordCore::RobotArmCommand::EType::eDJC;

    arm_cmd.positions_ = q;
    arm_cmd.speed_ = qd;
    arm_cmd.accelerations_ = qdd;
    arm_cmd.torque_ = torque;

    arm_cmd.seq_ = his_->sequence_number_;
    unsigned int n = his_->kord_->sendCommand(arm_cmd);
    his_->sequence_number_++;

    return n > 0;
}

bool ControlInterface::directTorqueControl(const std::array<double, 7UL> &torque)
{
    if (torque.size() != 7) {
        KORD_LOG_ERROR("ERROR: torque vector must have 7 elements");
        return false;
    }

    KordCore::RobotArmCommand arm_cmd;
    arm_cmd.command_type = KordCore::RobotArmCommand::EType::eDTC;

    arm_cmd.torque_ = torque;

    arm_cmd.seq_ = his_->sequence_number_;
    unsigned int n = his_->kord_->sendCommand(arm_cmd);
    his_->sequence_number_++;

    return n > 0;
}
bool ControlInterface::engageBrakes(const std::vector<int> &a_joints)
{
    token_t not_used_token_;
    return engageBrakes(a_joints, not_used_token_);
}

bool ControlInterface::engageBrakes(const std::vector<int> &a_joints, token_t &token)
{
    kord::KordCore::RobotArmCommand arm_cmd;

    arm_cmd.command_type = kord::KordCore::RobotArmCommand::EType::eFW;

    for (auto jnum : a_joints) {
        if (jnum < 1 || jnum > 7) {
            continue;
        }

        arm_cmd.fw_cmds_[jnum - 1] = EJointFirmwareCommand::eBrakeEngage;
    }

    arm_cmd.seq_ = his_->sequence_number_;
    int64_t command_token_id_;
    unsigned int n = his_->kord_->sendCommand(arm_cmd, command_token_id_);
    his_->sequence_number_++;

    if (n <= 0)
        return false;

    token = command_token_id_;

    return true;
}

bool ControlInterface::disengageBrakes(const std::vector<int> &a_joints)
{
    token_t not_used_token_;
    return disengageBrakes(a_joints, not_used_token_);
}

bool ControlInterface::disengageBrakes(const std::vector<int> &a_joints, token_t &token)
{
    kord::KordCore::RobotArmCommand arm_cmd;

    arm_cmd.command_type = kord::KordCore::RobotArmCommand::EType::eFW;

    for (auto jnum : a_joints) {
        if (jnum < 1 || jnum > 7) {
            continue;
        }

        arm_cmd.fw_cmds_[jnum - 1] = EJointFirmwareCommand::eBrakeDisengage;
    }

    arm_cmd.seq_ = his_->sequence_number_;
    int64_t command_token_id_;
    unsigned int n = his_->kord_->sendCommand(arm_cmd, command_token_id_);
    his_->sequence_number_++;

    if (n <= 0)
        return false;

    token = command_token_id_;

    return true;
}

int64_t ControlInterface::clearAlarmRequest(EClearRequest alarm_request)
{
    kord::KordCore::RobotSetupCommand set_cmd{};

    set_cmd.command_type = KordCore::RobotSetupCommand::EType::CLEAN_ALARM;
    set_cmd.alarm_id_ = alarm_request;

    set_cmd.seq_ = his_->sequence_number_;
    int64_t command_token_id_;
    unsigned int n = his_->kord_->sendCommand(set_cmd, command_token_id_);
    his_->sequence_number_++;

    if (n <= 0)
        return 0;

    return command_token_id_;
}

token_t ControlInterface::transmitRequest(const Request &a_req)
{
    token_t command_token_id_;
    his_->kord_->sendCommand(a_req, command_token_id_);
    return command_token_id_;
}
