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

#ifndef KR2_KORD_ITEM_IDS_H
#define KR2_KORD_ITEM_IDS_H

namespace kr2::kord::protocol {

/**
 * @brief Enumerates the data item identifiers used in the KORD protocol.
 *
 * This enumeration includes various command, request, response, and status items
 * that can be accessed or modified via the KORD interface. Each item is associated
 * with a unique identifier.
 */
enum class EKORDItemID : uint32_t { // clang-format off
    eNone                   = 0,        /**< Represents no data item */

    // COMMAND ITEMS
    eCommandMoveJ            = 0x0001,  /**< Command to move joint using MoveJ method */
    eCommandMoveL            = 0x0002,  /**< Command to move joint using MoveL method */
    eCommandMoveLQuat        = 0x0003,  /**< Command to move with quaternion support */
    eCommandManifold         = 0x0004,  /**< Command for manifold operations */
    eCommandMoveDirect       = 0x0005,  /**< Command to move directly */
    eCommandMoveVelocity     = 0x0006,  /**< Command to move with velocity control */
    eCommandMoveDirectTorque = 0x0007,  /**< Command to move directly with torque control */

    eCommandSetIODigitalOut = 0x0020,   /**< Command to set digital output I/O */
    eCommandSetIOAnalogOut  = 0x0021,   /**< Command to set analog output I/O */

    eCommandJointFirmware   = 0x0030,   /**< Command to update joint firmware */

    eCommandSetFrame        = 0x0040,   /**< Command to set frame parameters */
    eCommandSetLoad         = 0x0041,   /**< Command to set load parameters */
    eCommandCleanAlarm      = 0x0042,   /**< Command to clean alarms */
    eCommandRCAPI           = 0x0043,   /**< Command for RC API operations */

    eCommandBackdrive       = 0x0050,   /**< Command to backdrive the robot */

    // REQUEST ITEMS
    eRequestStatusArm       = 0x0100,   /**< Request status of the arm */
    eRequestIOStatus        = 0x0101,   /**< Request I/O status */
    eRequestJointTemp       = 0x0102,   /**< Request joint temperature */
    eRequestJointStatus     = 0x0103,   /**< Request joint status */
    eRequestSystem          = 0x0104,   /**< Request system status */
    eRequestTransfer        = 0x0105,   /**< Request data transfer */
    eRequestServer          = 0x0106,   /**< Request server information */
    eRequestServerStatus    = 0x0107,   /**< Request server status */
    eRequestVersion         = 0x0108,   /**< Request version information */

    // RESPONSE ITEMS
    eResponseIOStatus       = 0x0201,   /**< Response for I/O status request */
    eStatusEchoResponse     = 0x0300,   /**< Echo response for status */

    eResponse               = 0x0400,   /**< Generic response */
    eResponseServer         = 0x0401,   /**< Response related to server operations */
    eResponseTransfer       = 0x0402,   /**< Response related to data transfer */
    eResponseSystem         = 0x0403,   /**< Response related to system operations */

    eCustomData             = 0xf000,   /**< Custom data identifier */
}; // clang-format on

} // namespace kr2::kord::protocol

#endif // KR2_KORD_ITEM_IDS_H
