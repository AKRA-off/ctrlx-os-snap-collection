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

#ifndef KR2_KORD_SYSTEM_ALARMS_H
#define KR2_KORD_SYSTEM_ALARMS_H

#pragma once

namespace kr2::kord::protocol {


/**
 * @brief Category into which the condition ID falls into. Based on this category
 * the condition ID is interpreted.
 * 
 */
enum ESystemAlarmCategory {
    CAT_SAFETY_EVENT = 0x01,       //!< @brief Category for safety events - events that are triggered 
                                   //! by external devices most of the time, for example emregency 
                                   //1 or protective stop. 
    CAT_SOFT_STOP_EVENT = 0x02,    //1< @brief Category for soft stop events - events that are triggered 
                                   //! by the robot controller itself, for example a soft stop due to 
                                   //! a collision detection.
    CAT_HW_STAT = 0x03,            //!< @brief Category for hardware status events - events that are 
                                   //! triggered by the robot controller itself, mostly related to
                                   //! hardware status during initialization.
    CAT_CBUN_EVENT = 0x04          //!< @brief Category for CBun related alarms. 
};

/**
 * @brief Alarm context for system alarms. All safety events are related
 * to current system alarm output.
 * 
 */
enum ESystemAlarmContext {
    CNTXT_ESTOP = 0x01,     //!< @brief The emergency stop is flagged by the system.
    CNTXT_PSTOP = 0x02,     //!< @brief The protective stop is flagged by the system.
    CNTXT_SSTOP = 0x04,     //!< @brief The soft stop is flagged by the system. This is a 
                            //! non-critical stop related to controller itself.
    CNTXT_SYSERR = 0x08     //!< @brief This flag is set when there are hardware error bits set.
};

} // namespace kr2::kord::protocol

#endif //KR2_KORD_SYSTEM_ALARMS_H
