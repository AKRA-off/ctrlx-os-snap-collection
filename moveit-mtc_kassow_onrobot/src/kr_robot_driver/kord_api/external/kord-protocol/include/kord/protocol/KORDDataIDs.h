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

#ifndef KR2_KORD_DATA_IDS_H
#define KR2_KORD_DATA_IDS_H

#include <cstdint>

namespace kr2::kord::protocol {

/**
 * @brief A list of items that can be accessed or altered via the KORD interface.
 *
 * This enumeration contains READ, WRITE, and READ/WRITE items. Items can be retrieved
 * from the StatusFrame or ContentFrame's ContentItem.
 */
enum class EKORDDataID : uint16_t { // clang-format off
    eInvalid = 0, /**< Represents an invalid or uninitialized data ID */

    //
    // READ/WRITE ITEMS - Can be used to read data from status or to command the arm movement
    //

    // Joint Items
    eJConfigurationArm     = 0x01, /**< Configuration settings for the arm */
    eJAccelerationArm      = 0x02, /**< Acceleration data for the arm (Not available in joints protocol; RC API returns NaN) */
    eJSpeedArm             = 0x03, /**< Speed data for the arm */
    eJTorqueArm            = 0x04, /**< Torque data for the arm */
    eFrmTCPPose            = 0x05, /**< Pose information over TCP */
    eFrmTCPAcc             = 0x06, /**< Acceleration information over TCP */
    eFrmTCPVelocity        = 0x07, /**< Velocity information over TCP */
    eFrmTCPForce           = 0x08, /**< Force information over TCP (Dummy) */
    eJControlCMD           = 0x09, /**< Control commands for each joint */

    eFrmTCPQuaternion      = 0x12, /**< Quaternion data over TCP */

    // Velocity Move
    eVelMoveSync           = 0x0A, /**< Synchronization for velocity movement */
    eVelMovePeriod         = 0x0B, /**< Period for velocity movement */
    eVelMoveTimeout        = 0x0C, /**< Timeout for velocity movement */

    // Self-motion
    eManifoldJointSpeed    = 0x30, /**< Joint speed for self-motion */

    // Load Items
    eLoadId                = 0x0D, /**< Load identifier */
    eLoadPose              = 0x0E, /**< Pose information for the load */
    eLoadMass              = 0x0F, /**< Mass information for the load */
    eLoadCoG               = 0x10, /**< Center of Gravity for the load */
    eLoadInertia           = 0x11, /**< Inertia information for the load */

    // Frame Items
    eFrameId               = 0x20, /**< Frame identifier */
    eFramePoseRef          = 0x21, /**< Reference pose for the frame */
    eFramePose             = 0x22, /**< Pose information for the frame */

    //
    // READ ITEMS
    //

    // Joint Model Items
    eJConfigurationModel   = 0x101, /**< Configuration model for the joint */
    eJAccelerationModel    = 0x102, /**< Acceleration model for the joint */
    eJSpeedModel           = 0x103, /**< Speed model for the joint */
    eJTorqueModel          = 0x104, /**< Torque model for the joint */
    eJTorqueDeviation      = 0x13,  /**< Deviation in torque for the joint */
    eJStatorCurrent        = 0x105, /**< Stator current for the joint */
    eJStatorVoltage        = 0x106, /**< Stator voltage for the joint */
    eJSupplyVoltage        = 0x107, /**< Supply voltage for the joint */

    // Maintenance Items
    eJErrorBits            = 0x108, /**< Error bits for joint maintenance */
    eJStatusBits           = 0x109, /**< Status bits for joint maintenance */

    // Joint Temperature Items
    eJTemperatureBoard        = 0x110, /**< Temperature of the board */
    eJTemperatureJointEncoder = 0x111, /**< Temperature of the joint encoder */
    eJTemperatureRotorEncoder = 0x112, /**< Temperature of the rotor encoder */

    // Workspace Items
    // All frames referenced with respect to Base
    eFrmTFCModel          = 0x113, /**< TFC model frame */
    eFrmToolModel         = 0x114, /**< Tool model frame */
    eFrmElbowModel        = 0x115, /**< Elbow model frame */

    // IOB State Items
    eIODigitalInput        = 0x200, /**< Digital input state */
    eIOAnalogInput         = 0x201, /**< Analog input state */
    eIODigitalOutput       = 0x202, /**< Digital output state */
    eIOAnalogOutput        = 0x203, /**< Analog output state */
    eIOBSupplyVoltage1     = 0x204, /**< Supply voltage 1 */
    eIOBSupplyVoltage2     = 0x205, /**< Supply voltage 2 */
    eIOBCabinetTemperature = 0x206, /**< Cabinet temperature */
    eIOBErrorBits          = 0x207, /**< Error bits for IOB */
    eIOBStatusBits         = 0x208, /**< Status bits for IOB */

    eIODigitalValue          = 0x209, /**< Digital value */
    eIOAnalogMask            = 0x210, /**< Analog mask */
    eIODigitalSafetyConfig   = 0x211, /**< Digital safety configuration */
    eIODigitalSafetyMask     = 0x212, /**< Digital safety mask */

    // RC State Items
    eRCSafetyMode           = 0x300, /**< Safety mode for RC */
    eRCSafetyFlag           = 0x301, /**< Safety flag for RC */
    eRCMotionFlags          = 0x302, /**< Motion flags for RC */
    eRCMasterSpeed          = 0x303, /**< Master speed for RC */
    eEventsArray            = 0x304, /**< Events array for RC */
    eRCHWFlags              = 0x305, /**< Hardware flags for RC */
    eRCButtonFlags          = 0x306, /**< Button flags for RC */
    eRCSystemAlarmState     = 0x307, /**< System alarm state for RC */

    // Statistical Data Items
    eFailToReadEmpty        = 0x400, /**< Failure to read empty data */
    eFailToReadError        = 0x401, /**< Failure to read due to error */
    eMinDelay               = 0x402, /**< Minimum delay observed */
    eMaxDelay               = 0x403, /**< Maximum delay observed */
    eAverageDelay           = 0x404, /**< Average delay observed */
    eMinTickDelay           = 0x405, /**< Minimum tick delay observed */
    eMaxTickDelay           = 0x406, /**< Maximum tick delay observed */
    eAverageTickDelay       = 0x407, /**< Average tick delay observed */
    eMaxFramesInTick        = 0x408, /**< Maximum frames in a tick */
    eFaultyFramesStart      = 0x409, /**< Start of faulty frames */
    eSequenceNumber         = 0x410, /**< Sequence number */
    eSequenceNumberEcho     = 0x411, /**< Echo of the sequence number */
    eTxStamp                = 0x412, /**< Transmission timestamp */
    eTxStampEcho            = 0x413, /**< Echo of the transmission timestamp */
    // Last Command Status
    eLastCommandToken       = 0x414, /**< Token of the last command */
    eLastCommandErrorCode   = 0x415, /**< Error code of the last command */
    // CPU State Items
    eCPUStateId             = 0x416, /**< CPU state identifier */
    eCPUStateVal            = 0x417, /**< CPU state value */
    eRxStamp                = 0x418, /**< Reception timestamp */
    //
    eMaxCmdJitterWindow     = 0x419, /**< Maximum command jitter window */
    eAvgCmdJitterWindow     = 0x420, /**< Average command jitter window */
    eMaxCmdJitterGlobal     = 0x421, /**< Maximum global command jitter */
    eMaxTickDelayWindow     = 0x422, /**< Maximum tick delay window */
    eAvgTickDelayWindow     = 0x423, /**< Average tick delay window */
    eCmdLostWindowSeq       = 0x424, /**< Command lost window sequence */
    eCmdLostGlobalSeq       = 0x425, /**< Command lost global sequence */
    eMaxSysJitterWindow     = 0x426, /**< Maximum system jitter window */
    eAvgSysJitterWindow     = 0x427, /**< Average system jitter window */
    eMaxSysJitterGlobal     = 0x428, /**< Maximum global system jitter */
    eMaxTickDelayGlobal     = 0x429, /**< Maximum global tick delay */
    eCmdLostWindowTimestamp = 0x430, /**< Command lost window timestamp */
    eCmdLostGlobalTimestamp = 0x431, /**< Command lost global timestamp */
    //
    // WRITE ITEMS - used to alter desired items
    //

    // Tracking Control Items
    eTMovementType  = 0x500, /**< Type of movement for tracking control */
    eTBlendType     = 0x501, /**< Type of blend for tracking control */
    eTSyncType      = 0x502, /**< Type of synchronization for tracking control */
    eTMovementValue = 0x503, /**< Movement value for tracking control */
    eTBlendValue    = 0x504, /**< Blend value for tracking control */
    eTSyncValue     = 0x505, /**< Synchronization value for tracking control */
    eTOverlayType   = 0x506, /**< Overlay type for tracking control */

    // Control Items
    eCTRMovementMask     = 0x600, /**< Movement mask for control */
    eCTRCommandItem      = 0x601, /**< Command item for control */
    eCTRCommandTS        = 0x602, /**< Command timestamp for control */
    eCTRCommandStatus    = 0x603, /**< Command status for control */
    eCTRCommandErrorCode = 0x604, /**< Command error code for control */

    // Setup Items
    // Frame Setup
    eCTRSetFrameId       = 0x700, /**< Frame identifier setup */
    eCTRSetFrameRef      = 0x701, /**< Frame reference setup */
    eCTRSetFramePose     = 0x702, /**< Frame pose setup */
    // Load Setup
    eCTRSetLoadId        = 0x703, /**< Load identifier setup */
    eCTRSetLoadMass      = 0x704, /**< Load mass setup */
    eCTRSetLoadCoG       = 0x705, /**< Load center of gravity setup */
    eCTRSetLoadInertia   = 0x706, /**< Load inertia setup */
    // Alarm Clean
    eCTRCleanAlarmID     = 0x800, /**< Alarm identifier for cleaning */
    // Files Transfer
    eTransferMask         = 0x900, /**< Mask for file transfers */
    // RC API Command
    eRCAPICommandID      = 0x1000, /**< RC API command identifier */
    eRCAPICommandLength  = 0x1001, /**< Length of the RC API command */
    eRCAPICommandPayload = 0x1002, /**< Payload of the RC API command */

    // Backdrive Items
    eCTRBackdrive = 0x1100, /**< Backdrive command */

    // Server Services
    eServerServiceCommand  = 0x2000, /**< Server service command */
    eServerServiceId       = 0x2001, /**< Server service identifier */
    eServerPayload         = 0x2002, /**< Payload for server services */

    // Request Response Items
    eResponseType          = 0x3000, /**< Type of response */
    eResponseToken         = 0x3001, /**< Token for the response */
    eResponseCode          = 0x3002, /**< Code for the response */
    eResponsePayloadLength = 0x3003, /**< Payload length of the response */
    eResponsePayload       = 0x3010, /**< Payload of the response */

    // Frame Joint Poses
    eFrmJoint1Pose = 0x4000, /**< Pose of Joint 1 */
    eFrmJoint2Pose = 0x4001, /**< Pose of Joint 2 */
    eFrmJoint3Pose = 0x4002, /**< Pose of Joint 3 */
    eFrmJoint4Pose = 0x4003, /**< Pose of Joint 4 */
    eFrmJoint5Pose = 0x4004, /**< Pose of Joint 5 */
    eFrmJoint6Pose = 0x4005, /**< Pose of Joint 6 */
    eFrmJoint7Pose = 0x4006, /**< Pose of Joint 7 */

    eCRCValue            = 0x8000 /**< CRC value for data integrity */
}; // clang-format on

} // namespace kr2::kord::protocol

#endif // KR2_KORD_DATA_IDS_H
