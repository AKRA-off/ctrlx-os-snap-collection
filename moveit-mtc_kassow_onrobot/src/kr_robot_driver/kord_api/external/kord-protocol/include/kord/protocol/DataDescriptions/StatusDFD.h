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

#ifndef KR2_KORD_STATUS_FORMAT_DESCRIPTION_H
#define KR2_KORD_STATUS_FORMAT_DESCRIPTION_H

#include <kord/protocol/DataFormatDescription.h>
#include <kord/protocol/KORDDataIDs.h>

namespace kr2::kord::protocol {

class StatusArmReqFormatDescription_V0 : public DataFormatDescriptionItems {
public:
    StatusArmReqFormatDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber, EKORDDataID::eTxStamp, EKORDDataID::eCRCValue})
    {
    }
};

class StatusDataFormatDescription_V1 : public DataFormatDescriptionItems {
public:
    StatusDataFormatDescription_V1()
        : DataFormatDescriptionItems({EKORDDataID::eTxStampEcho,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eFailToReadError,
                                      EKORDDataID::eFailToReadEmpty,
                                      EKORDDataID::eMaxCmdJitterWindow,
                                      EKORDDataID::eAvgCmdJitterWindow,
                                      EKORDDataID::eMaxCmdJitterGlobal,
                                      EKORDDataID::eMaxTickDelayWindow, // i.e. round trip
                                      EKORDDataID::eAvgTickDelayWindow,
                                      EKORDDataID::eMaxTickDelayGlobal,
                                      EKORDDataID::eCmdLostWindowSeq,
                                      EKORDDataID::eCmdLostGlobalSeq,
                                      EKORDDataID::eCmdLostWindowTimestamp,
                                      EKORDDataID::eCmdLostGlobalTimestamp,
                                      EKORDDataID::eMaxSysJitterWindow,
                                      EKORDDataID::eAvgSysJitterWindow,
                                      EKORDDataID::eMaxSysJitterGlobal,
                                      EKORDDataID::eJConfigurationArm,
                                      EKORDDataID::eJSpeedArm,
                                      EKORDDataID::eJAccelerationArm,
                                      EKORDDataID::eJTorqueArm,
                                      EKORDDataID::eJTemperatureBoard,
                                      EKORDDataID::eJTemperatureJointEncoder,
                                      EKORDDataID::eJTemperatureRotorEncoder,
                                      EKORDDataID::eJErrorBits,
                                      EKORDDataID::eJStatusBits,
                                      EKORDDataID::eJStatorVoltage,
                                      EKORDDataID::eJStatorCurrent,
                                      EKORDDataID::eFrmTCPPose,
                                      EKORDDataID::eFrmTFCModel,
                                      EKORDDataID::eFrmJoint1Pose,
                                      EKORDDataID::eFrmJoint2Pose,
                                      EKORDDataID::eFrmJoint3Pose,
                                      EKORDDataID::eFrmJoint4Pose,
                                      EKORDDataID::eFrmJoint5Pose,
                                      EKORDDataID::eFrmJoint6Pose,
                                      EKORDDataID::eFrmJoint7Pose,
                                      EKORDDataID::eRCSafetyFlag,
                                      EKORDDataID::eRCHWFlags,
                                      EKORDDataID::eRCButtonFlags,
                                      EKORDDataID::eRCSystemAlarmState,
                                      EKORDDataID::eRCSafetyMode,
                                      EKORDDataID::eRCMasterSpeed,
                                      EKORDDataID::eRCMotionFlags,
                                      EKORDDataID::eIODigitalInput,
                                      EKORDDataID::eIODigitalOutput,
                                      EKORDDataID::eCTRCommandItem,
                                      EKORDDataID::eCTRCommandTS,
                                      EKORDDataID::eCTRCommandStatus,
                                      EKORDDataID::eCTRCommandErrorCode,
                                      EKORDDataID::eEventsArray,
                                      EKORDDataID::eIOBCabinetTemperature,
                                      EKORDDataID::eJTorqueModel,
                                      EKORDDataID::eLoadId,
                                      EKORDDataID::eLoadPose,
                                      EKORDDataID::eLoadMass,
                                      EKORDDataID::eLoadCoG,
                                      EKORDDataID::eLoadInertia,
                                      EKORDDataID::eFrameId,
                                      EKORDDataID::eFramePoseRef,
                                      EKORDDataID::eFramePose,
                                      EKORDDataID::eLastCommandToken,
                                      EKORDDataID::eLastCommandErrorCode,
                                      EKORDDataID::eCPUStateId,
                                      EKORDDataID::eCPUStateVal,
                                      EKORDDataID::eCRCValue,
                                      EKORDDataID::eIODigitalSafetyMask,
                                      EKORDDataID::eJTorqueDeviation})
    {
    }

    ~StatusDataFormatDescription_V1() override = default;
};

class StatusEchoResponseDescription_V0 : public DataFormatDescriptionItems {
public:
    StatusEchoResponseDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eTxStamp, EKORDDataID::eRxStamp, EKORDDataID::eCRCValue})
    {
    }
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_STATUS_FORMAT_DESCRIPTION_H
