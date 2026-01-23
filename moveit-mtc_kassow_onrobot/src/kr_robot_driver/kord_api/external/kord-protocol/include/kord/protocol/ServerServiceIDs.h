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

#ifndef KR2_KORD_SERVER_ITEMIDS_H
#define KR2_KORD_SERVER_ITEMIDS_H

#include <cstdint>

namespace kr2::kord::protocol {

/**
 * @brief This enum class defines the service IDs for the Controller server.
 *        it allows execution and monitoring of the server services.
 *
 *        To use these services, the teach pendant needs to be disconnected from
 *        the controller to enable tablet-less mode.
 *
 *        To Activate tablet-less mode, power down the controller and disconnect
 *        the teach pendant, then power up the controller. KORD API can now use the
 *        services bellow.
 *
 */
enum class EKORDServerServiceID : uint16_t { // clang-format off
    eLogCollection            = 0x0001, //!< @brief [UNUSED] Collects logs from the controller and uploads them to target location.
    eRobotUpdate              = 0x0002, //!< @brief [UNUSED] Updates the robot controller software.
    eModelUpdate              = 0x0003, //!< @brief [UNUSED]
    eProdInfoUpdate           = 0x0004, //!< @brief [UNUSED]
    eClusterInit              = 0x0005, //!< @brief [UNUSED]
    eFilesUpload              = 0x0006, //!< @brief [UNUSED]
    eFilesDownload            = 0x0007, //!< @brief [UNUSED]
    eCbunInstall              = 0x0008, //!< @brief [UNUSED]
    eFirmwareUpdate           = 0x0009, //!< @brief [UNUSED]
    eRobotZeroing             = 0x000A, //!< @brief [UNUSED]
    eSaveWorkcell             = 0x000B, //!< @brief [UNUSED]
    eLoadWorkcell             = 0x000C, //!< @brief [UNUSED]
    eCalibrationDataDownload  = 0x000D, //!< @brief [UNUSED]
    eCalibrationDataUpload    = 0x000E, //!< @brief [UNUSED]
    eFetchKincalData          = 0x000F, //!< @brief Fetches kinematic calibration data from the robot arm and
                                        //!         replaces the nominal robot model with kinematically calibrated model.
    eStartupConfigUpdate      = 0x0010, //!< @brief [UNUSED]
    eSoftwareUpdate           = 0x0011, //!< @brief Performs a software update.
}; // clang-format on

}

#endif // KR2_KORD_SERVER_ITEMIDS_H
