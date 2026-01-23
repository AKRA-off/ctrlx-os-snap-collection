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

#ifndef KR2_KORD_COMMAND_STATUS_FLAGS_H
#define KR2_KORD_COMMAND_STATUS_FLAGS_H

namespace kr2::kord::protocol {

/**
 * @brief Command status flag represents in what state command has ended up.
 *
 */
enum ECommandStatusFlags {
    COMMAND_STATUS_ACCEPTED =
        0, //!< @brief The command was successfully sent to controller. Note: it is just an indication that controller
           //!< has accepted the command. It does not however represent whether it was handled/finished successfully.
    COMMAND_STATUS_CBUN_ERROR = 1, //!< @brief The error occurred on KORD's side while processing command.
    COMMAND_STATUS_RC_API_ERROR = 2, //!< @brief The error occurred on responsive controller API's side.
    COMMAND_STATUS_CRC_MISMATCH = 3, //!< @brief The command was corrupted during transferring from KORD API to CBun, and CRC does not match.
    COMMAND_STATUS_COMMANDS_OVERLAP = 4, //!< @brief Too many commands were sent at once.
    COMMAND_STATUS_LOST = 5 //!< @brief The command was sent to RC API, but it did not confirm that controller accepted the command within last 20 ticks.
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_COMMAND_STATUS_FLAGS_H