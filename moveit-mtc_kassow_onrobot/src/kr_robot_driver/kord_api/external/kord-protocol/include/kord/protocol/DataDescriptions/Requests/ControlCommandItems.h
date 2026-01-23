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

#ifndef KR2_KORD_SYSTEM_REQUESTS_H
#define KR2_KORD_SYSTEM_REQUESTS_H

#pragma once

#include <cstdint>

namespace kr2::kord::protocol {

enum EControlCommandItems : uint16_t { // clang-format off
    // Operating system requests
    eInvalid = 0x0000, /**< Invalid control command item */
    eShutdown = 0x0001, /**< Shutdown the robot */
    eRestart = 0x0002, /**< Restart the robot */

    // Responsive controller requests
    eContinueDiscovery = 0x0101, /**< Continue discovery */
    eEnableBackDrive = 0x0102, /**< Enable back drive */
    eIOSet = 0x0103, /**< IO set */

    // Common requests
    eTransferLogFiles = 0x0201, /**< Transfer log files */
    eTransferDashboardJson = 0x0202, /**< Transfer dashboard JSON */
    eTransferCalibrationData = 0x0203, /**< Transfer calibration data */
    eServerEnableCommunication = 0x0204, /**< Enable communication */
    eServerDisableCommunication = 0x0205, /**< Disable communication */
    eGetVersion = 0x0206, /**< Get version */
    eSoftwareUpdate = 0x0207, /**< Software update */
    eGetSafetyZones = 0x0208, /**< Get safety zones */
    eGetRobotInfo = 0x0209, /**< Get robot info */

    // Files transfer requests
    eTransferFiles = 0x0300, /**< Transfer files */

    // RC API Command requests
    eRCAPICommand = 0x0400, /**< RC API command */

    // Server Request
    eServerRequest = 0x0500, /**< Server request */
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_SYSTEM_REQUESTS_H