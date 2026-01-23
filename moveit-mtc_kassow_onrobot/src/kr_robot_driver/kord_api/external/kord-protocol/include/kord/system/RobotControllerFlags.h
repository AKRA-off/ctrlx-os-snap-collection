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

#pragma once
#ifndef KR2_ROBOT_CONTROLLER_FLAGS_H
#define KR2_ROBOT_CONTROLLER_FLAGS_H

namespace kr2::kord::protocol {


/**
 * @brief Robot motion flags related to the internal robot controller. The controller is a software component tasked with maintaining
 * a constant stream of communication with hardware devices, including the IO Board, Joints, and ToolIO. Additionally,
 * the controller performs detection and initialization of these devices, and, if commanded to, updates their firmware.
 * Its main responsibilities are generating joint trajectory references, managing IO control, and ensuring the robot operates
 * within acceptable limits. The following flags describe its internal state.
 * 
 */
enum EMotionFlags {
    MOTION_FLAG_STANDBY = 1,         //!< @brief The controller is in standby mode, maintaining the arm at constant
                                     //! positions with zero velocities, accelerations, and torques to counteract gravity.
                                     //! At this stage, the controller is ready to receive new movement commands or
                                     //! to execute other tasks, such as firmware updates or entering backdrive mode.
    MOTION_FLAG_TRACKING = 2,        //!< @brief The controller has received a move command and is directing the joints
                                     //! to reach the target specified by the command, regardless of the current position.
                                     //! Event when commanding the robot to move to a position it is already at, this flag is set.
    MOTION_FLAG_TERMINATED = 4,      //!< @brief The controller is being terminated, which will soon result in the Teach Pendant (TP)
                                     //! displaying that the robot is unreachable. This state pertains only to the software;
                                     //! the controller cabinet remains powered on. Used exclusively for debugging to
                                     //! monitor a controller shutdown.
    MOTION_FLAG_HALT = 8,            //!< @brief The controller is in a HALT state due to the triggering of safety checks.
                                     //! Motion is stopped, and the robot will not resume or accept new tracking/motion commands
                                     //! until the event is acknowledged by the user.
    MOTION_FLAG_SYNC = 16,           //!< @brief Indicates the tracking has reached a synchronization point defined by the
                                     //! command, awaiting new data. This can occur when entering a blend zone or reaching
                                     //! a stop point, generally the controller is ready to execute a new command. If no new
                                     //! command is received, the robot will come to a standstill and the ::MOTION_FLAG_STANDBY
                                     //! will be set.
    MOTION_FLAG_SUSPENDED = 32,      //!< @brief The robot is in a suspended state. If it was moving, the movement is stopped but
                                     //! can be resumed, targeting the previously set destination. Suspension is triggered by
                                     //! toggling a button on the teach pendant or controller cabinet, indicated by a blinking
                                     //! green LED. To resume, press the physical button again or confirm via the teach pendant.
    MOTION_FLAG_OFFLINE = 64,        //!< @brief Reserved for testing and debugging, not expected in release instances. The controller
                                     //! transitions to ::MOTION_FLAG_INIT or ::MOTION_FLAG_REINIT almost immediately upon startup.
    MOTION_FLAG_INIT = 128,          //!< @brief The controller is initializing, possibly waiting for device responses or
                                     //! user interaction. If initialization times out, the flag ::HW_STAT_INIT_BLOCKED is set,
                                     //! with details in the hardware flags. Manual intervention is required to retry initialization.
    MOTION_FLAG_REINIT = 256,        //!< @brief Similar to ::MOTION_FLAG_INIT, used when the controller has already attempted
                                     //! device detection at least once.
    MOTION_FLAG_BACKDRIVE = 512,     //!< @brief Indicates the controller is in backdrive mode, ready for hand-guided operation.
                                     //! Positional references from the controller are disregarded, with torque as the only reference.
    MOTION_FLAG_PAUSED = 1024,       //!< @brief Movement is paused but can be resumed. Similar to ::MOTION_FLAG_SUSPENDED but
                                     //! triggered programmatically, not by a physical button.
    MOTION_FLAG_MAINTENANCE = 2048,  //!< @brief The controller is in maintenance mode for tasks such as firmware updates 
                                     //! or calibration handling. Intended for use by Kassow Robots support or under
                                     //! their direction.
    MOTION_FLAG_VELOCITYCTL = 4096,  //!< @brief The controller is in a velocity control mode, tracking a target velocity.
    MOTION_FLAG_ARTOACTIVE = 8192    //!< @brief Adaptive reference trajectory offset (ARTO) mode is active, allowing trajectory
                                     //! modification with additional signals, such as harmonic periodic inputs.
};

/**
 * @brief Robot safety flags indicate the controller or a device are not able to operate normally and the issue 
 * needs to be resolved in orders to continue. Most of the time the user needs to interact with the arm 
 * in order to properly handle reported event.
 * 
 */
enum ESafetyFlags {
    SAFETY_FLAG_UPDATE = 1,         //!<@brief Flag is set when there are changes in the safety flags.
    SAFETY_FLAG_ESTOP = 2,          //!<@brief Flag is set when there is an EStop in the system, originating
                                    //! from any of the devices, including EStop button on the cabinet or TP.
                                    //! The power supply is removed from the robot arm.
                                    //! The controller will report at the same time ::MOTION_FLAG_HALT
                                    //! and will not accept any movement commands.
    SAFETY_FLAG_PSTOP = 4,          //!<@brief Flag is set when there is a PStop in the system, originating
                                    //! from any of the devices, including PStop button on the cabinet or TP.
                                    //! The controller will keep the power supply to the robot arm, but will
                                    //! not accept any movement commands.
    SAFETY_FLAG_SSTOP = 8,          //!<@brief If the controller will detect violation of certain limits, it will
                                    //! set this flag. The robot will stop and will not accept any movement commands.
                                    //! The controller will also report the ::MOTION_FLAG_SUSPENDED flag.
    SAFETY_FLAG_USER_CONF_REQ = 16  //!<@brief If set, user interaction - either a button (if blinking) or on a tablet
                                    //! is needed. The interaction can be requested when there is a stop event.
};


/**
 * @brief Safety Mode flags reflect exactly the safety mode set by the user on the slider from the TP. Current options
 * are Normal, Reduced, and Safe. The safety mode is used to limit the robot movement speed the most and normal mode 
 * lets the robot operate unless its rated limits are exceeded.
 * 
 */
enum ESafetyMode {
    SAFETY_MODE_NORMAL = 1,     //!<@brief Robot movement is only restricted by its rated limits (cheetah).
    SAFETY_MODE_REDUCED = 2,    //!<@brief Robot fastest movement is reduced to max 1m/s (rabbit).
    SAFETY_MODE_SAFE = 3        //!<@brief Robot fastest movement is reduced to max 0.25m/s (turtle).
};

/**
 * @brief Hardware flags related to physical hardware devices - IO Board, Joints, and ToolIO.
 * 
 * These flags offer a partial insight into the device states or their interactions with the controller.
 * Their main purpose is to assist in detecting potential hardware issues and reporting them to the user.
 * 
 */
enum EHWFlags {
    HW_STAT_LOW_VOLTAGE_AFTER_RELAY_DETECTED  = 0x01,  //!< @brief Reports low voltage, indicating power 
                                                       //! should be supplied to the arm but is not. This 
                                                       //! suggests a hardware problem with the IO Board.
    HW_STAT_INIT_BLOCKED                      = 0x02,  //!< @brief Set when the initialization process is 
                                                       //! unable to proceed, requiring user intervention. 
                                                       //! Causes may include a hardware issue, such as a 
                                                       //! loose cable, or an inconsistent hardware state,
                                                       //! indicated by error or status bits.
    HW_STAT_INIT_RUID_MISMATCH                = 0x04,  //!< @brief Stops initialization due to an RUID mismatch,
                                                       //! meaning the controller has identified an arm 
                                                       //! that does not match the currently used one. This 
                                                       //! could occur when a new arm is connected or when 
                                                       //! the arm model is changed in the settings.
    HW_STAT_HARD_FAULT                        = 0x08,  //!< @brief Signals that the IO Board requires a 
                                                       //! power cycle to allow for reinitialization. The 
                                                       //! cause for this flag is outside the controller's scope.
    HW_STATUS_DISCOVERY_FAILED                = 0x0100,//!< @brief Indicates failure in device discovery during 
                                                       //! initialization. Further information can be found in 
                                                       //! the controller logs, accessible via the KORD API. 
                                                       //! This flag suggests a hardware issue, such as a loose 
                                                       //! cable or power failure in a device.
    HW_STATUS_ERROR_BITS_SET                  = 0x0200,//!< @brief Shows that error bits have been set on one 
                                                       //! of the devices. More details are available in the 
                                                       //! controller logs.
    HW_STATUS_DEVICE_DISABLED                 = 0x0400,//!< @brief Indicates a device has been disabled, used 
                                                       //! primarily for simulation and development.
    HW_STATUS_LOW_VOLTAGE                     = 0x0800,//!< @brief Indicates low voltage on the IO Board power 
                                                       //! output. Same meaning as ::HW_STAT_LOW_VOLTAGE_AFTER_RELAY_DETECTED.
    HW_STATUS_IOB_INIT_TIMEOUT                = 0x1000,//!< @brief IO Board initialization has timed out, 
                                                       //! suggesting the board was not discovered or has 
                                                       //! errors. Logs should provide more detail.
    HW_STATUS_SYNC_INIT_TIMEOUT               = 0x2000,//!< @brief Indicates a timeout during joint 
                                                       //! initialization, suggesting one or more joints 
                                                       //! were not discovered or have errors. Consult logs 
                                                       //! for more information.
    HW_STATUS_IOB_ESTOP_STALL                 = 0x4000,//!< @brief Occurs when the IO Board remains in EStop 
                                                       //! state despite expectations to proceed. Often 
                                                       //! related to a hard fault and requires a power cycle.
                                                       //! Persistent issues should be reported to support.
    HW_STATUS_IOB_PSTOP_STALL                 = 0x8000,//!< @brief Similar to HW_STATUS_IOB_ESTOP_STALL but 
                                                       //! related to PStop. Usually indicates other devices 
                                                       //! are not in the expected state, preventing 
                                                       //! initialization. Check error and status bits of all 
                                                       //! devices for diagnosis.
};


/**
 * @brief Button flags interpret the data related to buttons from the devices.
 * It does not reflect the physical buttons, but rather the state of the buttons.
 * For example a ::BUTTONS_FLAG_TOGGLE is set when the front button on the teach 
 * pendant is pressed as well when the button on the controller cabinet is pressed.
 * 
 * 
 */
enum EButtonFlags {
    BUTTONS_FLAG_ESTOP                        = 0x01, //!< @brief This flag reflects the error bits related to EStop switch.
                                                      //! It is set when the EStop switch is pressed, and the IO Board
                                                      //! is in the EStop state. It is also set when the EStop switch
                                                      //! is released, but the IO Board is still in the EStop state.
    BUTTONS_FLAG_PSTOP                        = 0x02, //!< @brief This flag reflects the status bits related to PStop switch.
                                                      //! It is set when the PStop switch is pressed, and the IO Board
                                                      //! is in the PStop state. It is also set when the PStop switch
                                                      //! is released, but the IO Board is still in the PStop state.
    BUTTONS_FLAG_TOGGLE                       = 0x04, //!< @brief Set if either the button on the teach pendant front
                                                      //! is pressed or the button on the controller cabinet is pressed.
                                                      //! It is present as long as the physical button is pressed down.
    BUTTONS_FLAG_BACKDRIVE                    = 0x08, //!< @brief Present if the button to backdrive the robot is pressed down.
                                                      //! It is the button on the ToolIO and the button on the back side 
                                                      //! of the teach pendant.
    BUTTONS_FLAG_TEACH                        = 0x10  //!< @brief Present if the teach button is pressed. It is a button 
                                                      //! on the back side of the teach pendant further away from the EStop.
};

} // namespace kr2::kord::protocol

#endif  // KR2_ROBOT_CONTROLLER_FLAGS_H