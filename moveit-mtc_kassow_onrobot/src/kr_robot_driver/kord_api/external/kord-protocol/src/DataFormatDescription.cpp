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

#include <kord/protocol/DataDescriptions/CommandsDFD.h>
#include <kord/protocol/DataDescriptions/RequestsDFD.h>
#include <kord/protocol/DataDescriptions/ResponsesDFD.h>
#include <kord/protocol/DataDescriptions/StatusDFD.h>
#include <kord/protocol/DataFormatDescription.h>
#include <kord/protocol/KORDFrames.h>

#include <cstring>
#include <functional>
#include <map>
#include <sstream>

using namespace kr2::kord::protocol;

template <typename Enumeration>
auto as_integer(Enumeration const value) -> typename std::underlying_type<Enumeration>::type
{
    return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

std::ostream &operator<<(std::ostream &a_os, const DataItem &a_di)
{
    a_os << "iid: " << a_di.did_ << ", T: " << as_integer(a_di.type_) << ", o: " << a_di.offset_;
    return a_os;
}

DataItem::DataItem() : did_(UINT32_MAX), type_(EKORDType::eVoid), offset_(0) {}

DataItem::DataItem(EKORDDataID a_iid, EKORDType a_type, unsigned int a_offset)
    : did_(static_cast<unsigned int>(a_iid)), type_(a_type), offset_(a_offset)
{
}

class DataFormatDescription::Internals {
public:
    Internals() = default;
    explicit Internals(unsigned int id);

    void pushItem(EKORDDataID a_item_id);

    [[nodiscard]] DataItem getItem(unsigned int a_item_idx) const;
    bool getItem(EKORDDataID a_item_id, DataItem &out) const;

private:
    [[nodiscard]] size_t itemSize(EKORDType item) const;
    [[nodiscard]] EKORDType itemType(EKORDDataID item) const;
    [[nodiscard]] unsigned int nextOffset() const;

public:
    uint32_t id_{};
    std::vector<DataItem> format_{};
    std::map<EKORDDataID, unsigned int> did_map_{};
    size_t max_data_len_{};

    static const size_t kr_size_lut[static_cast<unsigned long>(EKORDType::eTypeCount)];
    static const std::map<EKORDDataID, EKORDType> id2type;
};

DataFormatDescription::Internals::Internals(unsigned int a_id) : id_{a_id} {}

void DataFormatDescription::Internals::pushItem(EKORDDataID a_item_id)
{
    did_map_.insert(std::make_pair(a_item_id, format_.size()));
    format_.emplace_back(a_item_id, itemType(a_item_id), nextOffset());
    max_data_len_ = format_.back().offset_ + itemSize(format_.back().type_);
}

bool DataFormatDescription::Internals::getItem(EKORDDataID a_item_id, DataItem &a_out) const
{
    auto it = did_map_.find(a_item_id);

    if (it == did_map_.end()) {
        return false;
    }

    a_out = getItem(it->second);
    return true;
}

size_t DataFormatDescription::Internals::itemSize(EKORDType item) const
{
    return kr_size_lut[static_cast<unsigned long>(item)];
}

EKORDType DataFormatDescription::Internals::itemType(EKORDDataID a_item) const
{
    auto it = id2type.find(a_item);

    if (it != id2type.end()) {
        return it->second;
    }

    return EKORDType::eVoid;
}

unsigned int DataFormatDescription::Internals::nextOffset() const
{
    if (format_.empty())
        return 0;

    return format_.back().offset_ + itemSize(format_.back().type_);
}

const size_t DataFormatDescription::Internals::kr_size_lut[] = {
    0,  // eVoid = 0,
    1,  // eInt8,
    2,  // eInt16,
    4,  // eInt32,
    8,  // eInt64,
    1,  // eUInt8,
    2,  // eUInt16,
    4,  // eUInt32,
    8,  // eUInt64,
    8,  // eDouble,
    24, // eVector3d
    48, // eVector6d,
    56, // eVector7d,
    24, // eVector6i,
    28, // eVector7i
    14, // eVector7ui16
    28, // eVector7f
    98  // Byte Array size
};

// clang-format off
const std::map<EKORDDataID, EKORDType> DataFormatDescription::Internals::id2type{
    {EKORDDataID::eInvalid,                           EKORDType::eVoid},
    {EKORDDataID::eJConfigurationArm,	         EKORDType::eVector7d},
    {EKORDDataID::eJAccelerationArm,	                 EKORDType::eVector7d}, // eJAccelerationArm, // it is not available in joints protocol and therefore RC API returns nan
    {EKORDDataID::eJSpeedArm,	                 EKORDType::eVector7d}, // eJSpeedArm,
    {EKORDDataID::eJTorqueArm,	                 EKORDType::eVector7d}, // eJTorqueArm,
    {EKORDDataID::eFrmTCPPose,	                 EKORDType::eVector6d}, // eFrmTCPPose,
    {EKORDDataID::eFrmTCPQuaternion,                  EKORDType::eVector7d}, // eFrmTCPQuaternion,
    {EKORDDataID::eFrmTCPAcc,	                 EKORDType::eVector6d}, // eFrmTCPAcc,
    {EKORDDataID::eFrmTCPVelocity,                    EKORDType::eVector6d}, // eFrmTCPVelocity,
    {EKORDDataID::eFrmTCPForce,	                 EKORDType::eVector6d}, // eFrmTCPForce, // dummy
    {EKORDDataID::eJControlCMD,                       EKORDType::eVector7i}, // eJControlCMD - firmware commands

    {EKORDDataID::eJConfigurationModel,	         EKORDType::eVector7d}, // eJConfigurationModel,
    {EKORDDataID::eJSpeedModel,	                 EKORDType::eVector7d}, // eJSpeedModel,
    {EKORDDataID::eJAccelerationModel,	         EKORDType::eVector7d}, // eJAccelerationModel,
    {EKORDDataID::eJTorqueModel,	                 EKORDType::eVector7d}, // eJTorqueModel,
    {EKORDDataID::eJTorqueDeviation,                  EKORDType::eVector7f}, // eJTorqueDeviation,
    {EKORDDataID::eJStatorCurrent,	                 EKORDType::eVector7d}, // eJStatorCurrent,
    {EKORDDataID::eJStatorVoltage,	                 EKORDType::eVector7d}, // eJStatorVoltage,
    {EKORDDataID::eJSupplyVoltage,	                 EKORDType::eVector7d}, // eJSupplyVoltage,

    {EKORDDataID::eManifoldJointSpeed,                EKORDType::eDouble}, // eManifoldJointSpeed - for self-motion,

    {EKORDDataID::eJErrorBits,	                 EKORDType::eVector7i}, // eJErrorBits,
    {EKORDDataID::eJStatusBits,	                 EKORDType::eVector7i}, // eJStatusBits,

    {EKORDDataID::eJTemperatureBoard,	         EKORDType::eVector7ui16}, // eJTemperatureBoard,
    {EKORDDataID::eJTemperatureJointEncoder,	         EKORDType::eVector7ui16}, // eJTemperatureJointEncoder,
    {EKORDDataID::eJTemperatureRotorEncoder,	         EKORDType::eVector7ui16}, // eJTemperatureRotorEncoder,

    {EKORDDataID::eFrmTFCModel,	                 EKORDType::eVector6d}, // eFrmTFCModel,
    {EKORDDataID::eFrmToolModel,	         EKORDType::eVector6d}, // eFrmToolModel,
    {EKORDDataID::eFrmElbowModel,	         EKORDType::eVector6d}, // eFrmElbowModel,

    {EKORDDataID::eFrmJoint1Pose,	         EKORDType::eVector6d}, // eFrmJoint1Pose,
    {EKORDDataID::eFrmJoint2Pose,	         EKORDType::eVector6d}, // eFrmJoint2Pose,
    {EKORDDataID::eFrmJoint3Pose,	         EKORDType::eVector6d}, // eFrmJoint3Pose,
    {EKORDDataID::eFrmJoint4Pose,	         EKORDType::eVector6d}, // eFrmJoint4Pose,
    {EKORDDataID::eFrmJoint5Pose,	         EKORDType::eVector6d}, // eFrmJoint5Pose,
    {EKORDDataID::eFrmJoint6Pose,	         EKORDType::eVector6d}, // eFrmJoint6Pose,
    {EKORDDataID::eFrmJoint7Pose,	         EKORDType::eVector6d}, // eFrmJoint7Pose,

    {EKORDDataID::eIODigitalInput,               EKORDType::eUInt64},   // eIOBDigitalInput,
    {EKORDDataID::eIOAnalogInput,                EKORDType::eVector7d}, // eIOBAnalogInput,
    {EKORDDataID::eIODigitalOutput,	         EKORDType::eInt64},   // eIOBDigitalOutput,
    {EKORDDataID::eIOAnalogOutput,	         EKORDType::eVector6d}, // eIOBAnalogOutput,
    {EKORDDataID::eIOBSupplyVoltage1,	         EKORDType::eDouble},   // eIOBSupplyVoltage1,
    {EKORDDataID::eIOBSupplyVoltage2,	         EKORDType::eDouble},   // eIOBSupplyVoltage2,
    {EKORDDataID::eIOBCabinetTemperature,        EKORDType::eUInt16},   // eIOBCabinetTemperature,
    {EKORDDataID::eIOBErrorBits,	         EKORDType::eUInt32},   // eIOBErrorBits,
    {EKORDDataID::eIOBStatusBits,	         EKORDType::eUInt32},   // eIOBStatusBits,

    {EKORDDataID::eIODigitalValue,               EKORDType::eUInt8},    // eIODigitalValue,
    {EKORDDataID::eIODigitalSafetyConfig,        EKORDType::eInt32},    // eIODigitalSafetyConfig
    {EKORDDataID::eIODigitalSafetyMask,          EKORDType::eUInt32},   // eIODigitalSafetyMask

    {EKORDDataID::eRCSafetyMode,	         EKORDType::eUInt32},   // eRCSafetyMode,
    {EKORDDataID::eRCSafetyFlag,	         EKORDType::eUInt32},   // eRCSafetyFlag,
    {EKORDDataID::eRCMotionFlags,	         EKORDType::eUInt32},   // eRCMotionFlags,
    {EKORDDataID::eRCMasterSpeed,	         EKORDType::eDouble},   // eRCMasterSpeed,
    {EKORDDataID::eEventsArray,                  EKORDType::eByteArray},// Fixed size byte array
    {EKORDDataID::eRCHWFlags,	                 EKORDType::eUInt32},   // eRCHWFlags,
    {EKORDDataID::eRCButtonFlags,                EKORDType::eUInt32},   // eRCButtonFlags,
    {EKORDDataID::eRCSystemAlarmState,	         EKORDType::eUInt32},   // eRCSystemAlarmState,

    {EKORDDataID::eFailToReadEmpty,	         EKORDType::eUInt32},   // eFailToReadEmpty,
    {EKORDDataID::eFailToReadError,	         EKORDType::eUInt32},   // eFailToReadError,
    {EKORDDataID::eMinDelay,	                 EKORDType::eInt64},    // eMinDelay,
    {EKORDDataID::eMaxDelay,	                 EKORDType::eInt64},    // eMaxDelay,
    {EKORDDataID::eAverageDelay,	         EKORDType::eInt64},    // eAverageDelay,
    {EKORDDataID::eMinTickDelay,	         EKORDType::eInt64},    // eMinTickDelay,
    {EKORDDataID::eMaxTickDelay,	         EKORDType::eInt64},    // eMaxTickDelay,
    {EKORDDataID::eAverageTickDelay,	         EKORDType::eInt64},    // eAverageTickDelay,
    {EKORDDataID::eMaxFramesInTick,	         EKORDType::eInt16},    // eMaxFramesInTick,
    {EKORDDataID::eFaultyFramesStart,	         EKORDType::eInt64},    // eFaultyFramesStart,
    {EKORDDataID::eSequenceNumber,	         EKORDType::eUInt16},   // eSequenceNumber,
    {EKORDDataID::eSequenceNumberEcho,           EKORDType::eUInt16},   // eSequenceNumberEcho,
    {EKORDDataID::eTxStamp,	                 EKORDType::eInt64},    // eTxStamp,
    {EKORDDataID::eRxStamp,	                 EKORDType::eInt64},    // eExStamp,
    {EKORDDataID::eTxStampEcho,	                 EKORDType::eInt64},    // eTxStampEcho,
    {EKORDDataID::eTMovementType,                EKORDType::eUInt8},    // eTMovementType,
    {EKORDDataID::eTBlendType,	                 EKORDType::eUInt8},    // eTBlendType,
    {EKORDDataID::eTSyncType,	                 EKORDType::eUInt8},    // eTSyncType,
    {EKORDDataID::eTMovementValue,	         EKORDType::eDouble},   // eTMovementValue,
    {EKORDDataID::eTBlendValue,	                 EKORDType::eDouble},   // eTBlendValue,
    {EKORDDataID::eTSyncValue,	                 EKORDType::eDouble},   // eTSyncValue,
    {EKORDDataID::eTOverlayType,	         EKORDType::eUInt8},    // eTOverlayType
    {EKORDDataID::eCTRMovementMask,              EKORDType::eUInt8},    // To switch dynamic/velocity/position control
    {EKORDDataID::eCTRCommandItem,               EKORDType::eUInt16},
    {EKORDDataID::eCTRCommandTS,                 EKORDType::eInt64},
    {EKORDDataID::eCTRCommandStatus,             EKORDType::eUInt16},
    {EKORDDataID::eCTRCommandErrorCode,          EKORDType::eUInt32},    // Returned after failure of the request
    {EKORDDataID::eCRCValue,                     EKORDType::eUInt16},

    {EKORDDataID::eMaxCmdJitterWindow,	         EKORDType::eInt64},    // eMaxCmdJitterWindow,
    {EKORDDataID::eAvgCmdJitterWindow,	         EKORDType::eInt64},    // eAvgCmdJitterWindow,
    {EKORDDataID::eMaxCmdJitterGlobal,	         EKORDType::eInt64},    // eMaxCmdJitterGlobal,
    {EKORDDataID::eMaxTickDelayWindow,	         EKORDType::eInt64},    // eMaxTickDelayWindow,
    {EKORDDataID::eAvgTickDelayWindow,	         EKORDType::eInt64},    // eAvgTickDelayWindow,
    {EKORDDataID::eMaxTickDelayGlobal,	         EKORDType::eInt64},    // eMaxTickDelayGlobal,
    {EKORDDataID::eCmdLostWindowSeq,	         EKORDType::eInt64},    // eCmdLostWindowSeq,
    {EKORDDataID::eCmdLostGlobalSeq,	         EKORDType::eInt64},    // eCmdLostGlobalSeq,
    {EKORDDataID::eCmdLostWindowTimestamp,	 EKORDType::eInt64},    // eCmdLostWindowTimestmp,
    {EKORDDataID::eCmdLostGlobalTimestamp,	 EKORDType::eInt64},    // eCmdLostGlobalTimestmp,

    {EKORDDataID::eMaxSysJitterWindow,	         EKORDType::eInt32},    // eMaxSysJitterWindow,
    {EKORDDataID::eAvgSysJitterWindow,	         EKORDType::eInt32},    // eAvgSysJitterWindow,
    {EKORDDataID::eMaxSysJitterGlobal,	         EKORDType::eInt32},    // eMaxSysJitterGlobal,

    {EKORDDataID::eVelMoveSync,                  EKORDType::eUInt32},   // eVelMoveSync
    {EKORDDataID::eVelMovePeriod,                EKORDType::eDouble},   // eVelMovePeriod
    {EKORDDataID::eVelMoveTimeout,               EKORDType::eDouble},   // eVelMoveTimeout

    {EKORDDataID::eFrameId,	                 EKORDType::eUInt8},    // eFrameId,
    {EKORDDataID::eFramePoseRef,	         EKORDType::eUInt8},    // eFramePoseRef,
    {EKORDDataID::eFramePose,	         EKORDType::eVector6d}, // eFramePose,

    {EKORDDataID::eLoadId,	                 EKORDType::eUInt8},    // eLoadId,
    {EKORDDataID::eLoadPose,	                 EKORDType::eVector6d}, // eLoadPose,
    {EKORDDataID::eLoadMass,	                 EKORDType::eDouble},   // eLoadMass,
    {EKORDDataID::eLoadCoG,	                 EKORDType::eVector3d}, // eLoadCoG,
    {EKORDDataID::eLoadInertia,	         EKORDType::eVector6d}, // eLoadInertia,

    {EKORDDataID::eCTRSetFrameId,             EKORDType::eUInt8},    // eCTRSetFrameId,
    {EKORDDataID::eCTRSetFrameRef,	         EKORDType::eUInt8},    // eCTRSetFrameRef,
    {EKORDDataID::eCTRSetFramePose,	         EKORDType::eVector6d}, // eCTRSetFramePose,

    {EKORDDataID::eCTRSetLoadId,	         EKORDType::eUInt8},    // eCTRSetLoadId,
    {EKORDDataID::eCTRSetLoadMass,	         EKORDType::eDouble},   // eCTRSetLoadMass,
    {EKORDDataID::eCTRSetLoadCoG,	         EKORDType::eVector3d}, // eCTRSetLoadCoG,
    {EKORDDataID::eCTRSetLoadInertia,  	         EKORDType::eVector6d}, // eCTRSetLoadInertia,

    {EKORDDataID::eCTRCleanAlarmID,	         EKORDType::eUInt8},     // eCTRCleanAlarmID
    {EKORDDataID::eCPUStateId,                   EKORDType::eUInt8},     // eCPUStateId
    {EKORDDataID::eCPUStateVal,                  EKORDType::eUInt16},    // eCPUStateVal

    {EKORDDataID::eLastCommandToken,	         EKORDType::eUInt64},    // eLastCommandToken,
    {EKORDDataID::eLastCommandErrorCode,	 EKORDType::eInt8},      // eLastCommandErrorCode

    {EKORDDataID::eTransferMask,	                 EKORDType::eUInt32},    // eTransferMask

    {EKORDDataID::eRCAPICommandID,	         EKORDType::eUInt16},         // eRCAPICommandID
    {EKORDDataID::eRCAPICommandLength,	         EKORDType::eUInt32},        // eRCAPICommandLength
    {EKORDDataID::eRCAPICommandPayload,	         EKORDType::eByteArray},     // eRCAPICommandPayload

    {EKORDDataID::eServerServiceCommand,         EKORDType::eUInt16}, // eServerServiceCommand
    {EKORDDataID::eServerServiceId,              EKORDType::eUInt16}, // eServerServiceId
    {EKORDDataID::eServerPayload,                EKORDType::eByteArray}, // eServerPayload

    {EKORDDataID::eResponseType,                 EKORDType::eUInt16}, // eResponsePayload
    {EKORDDataID::eResponseToken,                EKORDType::eUInt64}, // eResponsePayload
    {EKORDDataID::eResponseCode,                 EKORDType::eUInt16}, // eResponsePayload
    {EKORDDataID::eResponsePayload,              EKORDType::eByteArray}, // eResponsePayload
    {EKORDDataID::eResponsePayloadLength,        EKORDType::eUInt16}, // eResponsePayload

}; // clang-format on

DataFormatDescription::DataFormatDescription() : his_(new Internals) {}

DataFormatDescription::DataFormatDescription(unsigned int a_id) : his_(new Internals(a_id)) {}

DataFormatDescription::~DataFormatDescription() { delete his_; }

DataFormatDescription &DataFormatDescription::setID(unsigned int a_id)
{
    his_->id_ = a_id;
    return *this;
}

DataFormatDescription &DataFormatDescription::addItem(EKORDDataID a_item)
{
    his_->pushItem(a_item);
    return *this;
}

uint32_t DataFormatDescription::ID() const { return his_->id_; }

const std::vector<DataItem> &DataFormatDescription::describeFormat() const { return his_->format_; }

std::string DataFormatDescription::asString() const
{
    std::stringstream ss;

    ss << "FID: " << his_->id_ << "[\n";
    int i = 0;
    for (const auto &item : his_->format_) {
        ss << "    " << i << ": ( " << item << " ),\n";
        i++;
    }
    ss << "]";

    return ss.str();
}

unsigned int DataFormatDescription::getItemsCount() const { return his_->format_.size(); }

const std::vector<DataItem> &DataFormatDescription::items() const { return his_->format_; }

DataItem DataFormatDescription::getItem(unsigned int a_item_idx) const { return his_->getItem(a_item_idx); }

DataItem DataFormatDescription::Internals::getItem(unsigned int a_item_idx) const
{
    if (a_item_idx > format_.size()) {
        return {};
    }

    return format_[a_item_idx];
}

bool DataFormatDescription::getItem(EKORDDataID a_id, DataItem &a_out_item) const
{
    return his_->getItem(a_id, a_out_item);
}

unsigned int DataFormatDescription::getOffset(EKORDDataID a_id) const
{
    DataItem item;

    if (his_->getItem(a_id, item)) {
        return item.offset_;
    }

    return UINT32_MAX;
}

EKORDType DataFormatDescription::getType(EKORDDataID a_id) const
{
    DataItem item;

    if (his_->getItem(a_id, item)) {
        return item.type_;
    }

    return EKORDType::eVoid;
}

size_t DataFormatDescription::getMaxDataLength() const { return his_->max_data_len_; }

void DataFormatDescription::serialize(std::vector<uint8_t> &a_buffer)
{
    auto *ptr = reinterpret_cast<uint8_t *>(&his_->id_);
    a_buffer.push_back(*ptr);
    ptr++;
    a_buffer.push_back(*ptr);
    ptr++;
    a_buffer.push_back(*ptr);
    ptr++;
    a_buffer.push_back(*ptr);

    a_buffer.resize(his_->format_.size() * sizeof(DataItem) + 4);
    std::memcpy(a_buffer.data() + 4, his_->format_.data(), his_->format_.size() * sizeof(DataItem));
}

void DataFormatDescription::deserialize(std::vector<uint8_t> a_buffer)
{
    union temp {
        uint32_t id;
        uint8_t buf[4];
    } conv{};

    conv.buf[0] = a_buffer[0];
    conv.buf[1] = a_buffer[1];
    conv.buf[2] = a_buffer[2];
    conv.buf[3] = a_buffer[3];

    his_->id_ = conv.id;

    size_t len = (a_buffer.size() - 4) / sizeof(DataItem);
    his_->format_.resize(len);
    std::memcpy(his_->format_.data(), a_buffer.data() + 4, (a_buffer.size() - 4));
}

DataFormatDescription DataFormatDescription::makeStatusFrameDescription(unsigned int a_version)
{
    DataFormatDescription dfd;
    DataFormatDescriptionItems dfdb;

    dfd.setID(kr2::kord::protocol::KORD_FRAME_ID_STATUS);

    switch (a_version) {
    // NEWER Versions
    // case :
    //     /* code */
    //     break;
    case kr2::kord::protocol::KORD_STATUS_VERSION_0:
    case kr2::kord::protocol::KORD_STATUS_VERSION_1:
    default:
        dfdb = StatusDataFormatDescription_V1();
        break;
    }

    for (const auto &item : dfdb.getItems()) {
        dfd.addItem(item);
    }

    return dfd;
}

DataFormatDescription DataFormatDescription::makeItemDescriptionLatest(EKORDItemID a_iid)
{
    DataFormatDescription dfd;

    static const std::map<EKORDItemID, std::function<DataFormatDescriptionItems()>> item_map = {
        {EKORDItemID::eCommandMoveJ, [] { return CommandMoveJDescription_V0(); }},
        {EKORDItemID::eCommandMoveL, [] { return CommandMoveLDescription_V0(); }},
        {EKORDItemID::eCommandMoveLQuat, [] { return CommandMoveLQuatDescription_V0(); }},
        {EKORDItemID::eCommandManifold, [] { return CommandMoveManifoldDescription_V0(); }},
        {EKORDItemID::eRequestStatusArm, [] { return StatusArmReqFormatDescription_V0(); }},
        {EKORDItemID::eCommandJointFirmware, [] { return CommandJointFirmwareDescription_V0(); }},
        {EKORDItemID::eRequestSystem, [] { return RequestSystemDescription_V0(); }},
        {EKORDItemID::eRequestTransfer, [] { return RequestTransferDescription_V0(); }},
        {EKORDItemID::eRequestServer, [] { return CommandServerServiceDescription_V0(); }},
        {EKORDItemID::eStatusEchoResponse, [] { return StatusEchoResponseDescription_V0(); }},
        {EKORDItemID::eCommandSetIODigitalOut, [] { return RequestIOSetDescription_V0(); }},
        {EKORDItemID::eCommandMoveDirect, [] { return CommandMoveDirectDescription_V0(); }},
        {EKORDItemID::eCommandMoveDirectTorque, [] { return CommandMoveDirectTorqueDescription_V0(); }},
        {EKORDItemID::eCommandMoveVelocity, [] { return CommandMoveVelocityDescription_V0(); }},
        {EKORDItemID::eCommandSetFrame, [] { return CommandSetFrameDescription_V0(); }},
        {EKORDItemID::eCommandSetLoad, [] { return CommandSetLoadDescription_V0(); }},
        {EKORDItemID::eCommandCleanAlarm, [] { return CommandCleanAlarmDescription_V0(); }},
        {EKORDItemID::eCommandRCAPI, [] { return CommandRCAPICommandDescription_V0(); }},
        {EKORDItemID::eResponse, [] { return ResponseDescription_V0(); }},
        {EKORDItemID::eResponseSystem, [] { return ResponseDescription_V0(); }},
        {EKORDItemID::eResponseServer, [] { return ResponseDescription_V0(); }}};

    auto it = item_map.find(a_iid);
    if (it != item_map.end()) {
        const DataFormatDescriptionItems &data_items = it->second();
        for (const auto &item : data_items.getItems()) {
            dfd.addItem(item);
        }
    }

    return dfd;
}