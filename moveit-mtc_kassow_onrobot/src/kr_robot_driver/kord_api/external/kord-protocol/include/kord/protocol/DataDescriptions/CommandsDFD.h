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

#ifndef KR2_KORD_COMMANDS_FORMAT_DESCRIPTION_H
#define KR2_KORD_COMMANDS_FORMAT_DESCRIPTION_H

#pragma once

#include <kord/protocol/DataFormatDescription.h>
#include <kord/protocol/KORDDataIDs.h>

namespace kr2::kord::protocol {

class CommandMoveJDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandMoveJDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eCTRMovementMask,
                                      EKORDDataID::eJConfigurationArm,
                                      EKORDDataID::eJAccelerationArm,
                                      EKORDDataID::eJSpeedArm,
                                      EKORDDataID::eJTorqueArm,
                                      EKORDDataID::eTMovementType,
                                      EKORDDataID::eTBlendType,
                                      EKORDDataID::eTSyncType,
                                      EKORDDataID::eTMovementValue,
                                      EKORDDataID::eTBlendValue,
                                      EKORDDataID::eTSyncValue,
                                      EKORDDataID::eTOverlayType,
                                      EKORDDataID::eCRCValue})
    {
    }
};

class CommandMoveLDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandMoveLDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eCTRMovementMask,
                                      EKORDDataID::eFrmTCPPose,
                                      EKORDDataID::eFrmTCPAcc,
                                      EKORDDataID::eFrmTCPVelocity,
                                      EKORDDataID::eFrmTCPForce,
                                      EKORDDataID::eTMovementType,
                                      EKORDDataID::eTBlendType,
                                      EKORDDataID::eTSyncType,
                                      EKORDDataID::eTMovementValue,
                                      EKORDDataID::eTBlendValue,
                                      EKORDDataID::eTSyncValue,
                                      EKORDDataID::eTOverlayType,
                                      EKORDDataID::eCRCValue})
    {
    }
};

class CommandMoveLQuatDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandMoveLQuatDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eCTRMovementMask,
                                      EKORDDataID::eFrmTCPQuaternion,
                                      EKORDDataID::eTMovementType,
                                      EKORDDataID::eTBlendType,
                                      EKORDDataID::eTSyncType,
                                      EKORDDataID::eTMovementValue,
                                      EKORDDataID::eTBlendValue,
                                      EKORDDataID::eTSyncValue,
                                      EKORDDataID::eTOverlayType,
                                      EKORDDataID::eCRCValue})
    {
    }
};

class CommandJointFirmwareDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandJointFirmwareDescription_V0()
        : DataFormatDescriptionItems(
              {EKORDDataID::eSequenceNumber, EKORDDataID::eTxStamp, EKORDDataID::eJControlCMD, EKORDDataID::eCRCValue})
    {
    }
};

class CommandMoveDirectDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandMoveDirectDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eJConfigurationArm,
                                      EKORDDataID::eJSpeedArm,
                                      EKORDDataID::eJAccelerationArm,
                                      EKORDDataID::eJTorqueArm,
                                      EKORDDataID::eCRCValue})
    {
    }
};

class CommandMoveDirectTorqueDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandMoveDirectTorqueDescription_V0()
        : DataFormatDescriptionItems(
              {EKORDDataID::eSequenceNumber, EKORDDataID::eTxStamp, EKORDDataID::eJTorqueArm, EKORDDataID::eCRCValue})
    {
    }
};

class CommandMoveVelocityDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandMoveVelocityDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eFrmTCPVelocity,
                                      EKORDDataID::eVelMoveSync,
                                      EKORDDataID::eVelMovePeriod,
                                      EKORDDataID::eVelMoveTimeout,
                                      EKORDDataID::eCRCValue})
    {
    }
};

class CommandMoveManifoldDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandMoveManifoldDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eManifoldJointSpeed,
                                      EKORDDataID::eCRCValue})
    {
    }
};

class CommandSetFrameDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandSetFrameDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eCTRSetFrameId,
                                      EKORDDataID::eCTRSetFrameRef,
                                      EKORDDataID::eCTRSetFramePose,
                                      EKORDDataID::eCRCValue})
    {
    }
};

class CommandSetLoadDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandSetLoadDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eCTRSetLoadId,
                                      EKORDDataID::eCTRSetLoadMass,
                                      EKORDDataID::eCTRSetLoadCoG,
                                      EKORDDataID::eCTRSetLoadInertia,
                                      EKORDDataID::eCRCValue})
    {
    }
};

class CommandCleanAlarmDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandCleanAlarmDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eCTRCleanAlarmID,
                                      EKORDDataID::eCRCValue})
    {
    }
};

// RCApi
class CommandRCAPICommandDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandRCAPICommandDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eRCAPICommandID,
                                      EKORDDataID::eRCAPICommandLength,
                                      EKORDDataID::eRCAPICommandPayload,
                                      EKORDDataID::eCRCValue})
    {
    }
};

// Server services
class CommandServerServiceDescription_V0 : public DataFormatDescriptionItems {
public:
    CommandServerServiceDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eServerServiceCommand,
                                      EKORDDataID::eServerServiceId,
                                      EKORDDataID::eServerPayload,
                                      EKORDDataID::eCRCValue})
    {
    }
};

class CommandBackdriveDescription_V0 : public DataFormatDescriptionItems {
    public:
    CommandBackdriveDescription_V0()
        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
                                      EKORDDataID::eTxStamp,
                                      EKORDDataID::eCTRBackdrive,
                                      EKORDDataID::eCRCValue})
    {
    }
};

// class CommandSetOutputIODigitalDescription_V0 : public DataFormatDescriptionItems {
// public:
//    CommandSetOutputIODigitalDescription_V0()
//        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
//                                      EKORDDataID::eTxStamp,
//                                      EKORDDataID::eCTRCommandItem,
//                                      EKORDDataID::eIODigitalValue,
//                                      EKORDDataID::eCRCValue})
//    {
//    }
//};
//
// class CommandSetOutputIORelayDescription_V0 : public DataFormatDescriptionItems {
// public:
//    CommandSetOutputIORelayDescription_V0()
//        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
//                                      EKORDDataID::eTxStamp,
//                                      EKORDDataID::eCTRCommandItem,
//                                      EKORDDataID::eIODigitalValue,
//                                      EKORDDataID::eCRCValue})
//    {
//    }
//};
//
// class CommandSetOutputIODigitalDescription_V0 : public DataFormatDescriptionItems {
// public:
//    CommandSetOutputIODigitalDescription_V0()
//        : DataFormatDescriptionItems({EKORDDataID::eSequenceNumber,
//                                      EKORDDataID::eTxStamp,
//                                      EKORDDataID::eCTRCommandItem,
//                                      EKORDDataID::eIODigitalValue,
//                                      EKORDDataID::eCRCValue})
//    {
//    }
//};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_COMMANDS_FORMAT_DESCRIPTION_H