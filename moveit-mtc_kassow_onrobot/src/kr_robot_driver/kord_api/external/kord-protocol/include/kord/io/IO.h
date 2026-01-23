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

#ifndef KR2_KORD_PROTOCOL_IO_H
#define KR2_KORD_PROTOCOL_IO_H

#define MASK_RELAY1 1 << 0
#define MASK_RELAY2 1 << 1
#define MASK_RELAY3 1 << 2
#define MASK_RELAY4 1 << 3

#define MASK_DO1 1 << 4
#define MASK_DO2 1 << 5
#define MASK_DO3 1 << 6
#define MASK_DO4 1 << 7
#define MASK_DO5 1 << 8
#define MASK_DO6 1 << 9
#define MASK_DO7 1 << 10
#define MASK_DO8 1 << 11

#define MASK_TB1 1 << 12
#define MASK_TB2 1 << 13
#define MASK_TB3 1 << 14
#define MASK_TB4 1 << 15

#define MASK_SDO1 1 << 16
#define MASK_SDO2 1 << 17
#define MASK_SDO3 1 << 18
#define MASK_SDO4 1 << 19

enum ESafePortConfiguration {
    eSafePortDisabled = 1,
    eSafePortEnabled = 2,
    eSafePortPStopMapped = 3,
    eSafePortEStopMapped = 4,
    eSafePortBothMapped = 5
};

#endif