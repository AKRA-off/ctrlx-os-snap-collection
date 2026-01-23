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

#ifndef KR2_KORD_IO_REQUEST_H
#define KR2_KORD_IO_REQUEST_H

#pragma once

#include <kord/api/api_request.h>
#include <kord/io/IO.h>
#include <kord/protocol/KORDItemIDs.h>

namespace kr2::kord {

/**
 * @class RequestIO
 * @brief Represents an I/O request in the KORD API.
 *
 * The RequestIO class extends the base Request class to handle input/output operations,
 * including setting digital and analog outputs, and configuring safe ports.
 */
class RequestIO : public Request {
public:
    /**
     * @enum DIGITAL_RELAYS
     * @brief Enumeration for digital relay masks.
     */
    enum DIGITAL_RELAYS {
        RELAY1 = MASK_RELAY1, /**< @brief Mask for Relay 1 */
        RELAY2 = MASK_RELAY2, /**< @brief Mask for Relay 2 */
        RELAY3 = MASK_RELAY3, /**< @brief Mask for Relay 3 */
        RELAY4 = MASK_RELAY4  /**< @brief Mask for Relay 4 */
    };

    /**
     * @enum DIGITAL_IOBOARD
     * @brief Enumeration for digital I/O board masks.
     */
    enum DIGITAL_IOBOARD {
        DO1 = MASK_DO1, /**< @brief Mask for Digital Output 1 */
        DO2 = MASK_DO2, /**< @brief Mask for Digital Output 2 */
        DO3 = MASK_DO3, /**< @brief Mask for Digital Output 3 */
        DO4 = MASK_DO4, /**< @brief Mask for Digital Output 4 */
        DO5 = MASK_DO5, /**< @brief Mask for Digital Output 5 */
        DO6 = MASK_DO6, /**< @brief Mask for Digital Output 6 */
        DO7 = MASK_DO7, /**< @brief Mask for Digital Output 7 */
        DO8 = MASK_DO8  /**< @brief Mask for Digital Output 8 */
    };

    /**
     * @enum DIGITAL_IOTOOLB
     * @brief Enumeration for digital I/O tool board masks.
     */
    enum DIGITAL_IOTOOLB {
        TB1 = MASK_TB1, /**< @brief Mask for Tool Board 1 */
        TB2 = MASK_TB2, /**< @brief Mask for Tool Board 2 */
        TB3 = MASK_TB3, /**< @brief Mask for Tool Board 3 */
        TB4 = MASK_TB4  /**< @brief Mask for Tool Board 4 */
    };

    /**
     * @enum DIGITAL_SAFE
     * @brief Enumeration for digital safe port masks.
     */
    enum DIGITAL_SAFE {
        SDO1 = MASK_SDO1, /**< @brief Mask for Safe Digital Output 1 */
        SDO2 = MASK_SDO2, /**< @brief Mask for Safe Digital Output 2 */
        SDO3 = MASK_SDO3, /**< @brief Mask for Safe Digital Output 3 */
        SDO4 = MASK_SDO4  /**< @brief Mask for Safe Digital Output 4 */
    };

    /**
     * @brief Bitmask representing the I/O ports.
     */
    int64_t mask_{}; /**< @brief Bitmask for specifying I/O ports to be enabled or disabled. */

    /**
     * @brief Value to be set for analog outputs.
     */
    uint8_t value_{}; /**< @brief Value to set for analog output operations. */

    /**
     * @brief Configuration identifier for safe ports.
     */
    int32_t config_id_{}; /**< @brief Identifier for configuring safe ports. */

    /**
     * @brief Default constructor that initializes the RequestIO object.
     *
     * Initializes the request type to I/O operations and sets default values for member variables.
     */
    RequestIO();

    /**
     * @brief Constructs a RequestIO object by copying from a base Request object.
     *
     * @param a_req The base Request object to copy from.
     */
    explicit RequestIO(const Request &a_req);

    /**
     * @brief Configures the request to set digital outputs.
     *
     * @return Reference to the modified RequestIO object.
     */
    RequestIO &asSetIODigitalOut();

    /**
     * @brief Configures the request to set analog outputs.
     *
     * @return Reference to the modified RequestIO object.
     */
    // RequestIO &asSetIOAnalogOut();

    /**
     * @brief Enables specified I/O ports by setting their corresponding masks.
     *
     * @param mask Bitmask representing the I/O ports to enable.
     * @return Reference to the modified RequestIO object.
     */
    RequestIO &withEnabledPorts(int64_t mask);

    /**
     * @brief Disables specified I/O ports by clearing their corresponding masks.
     *
     * @param mask Bitmask representing the I/O ports to disable.
     * @return Reference to the modified RequestIO object.
     */
    RequestIO &withDisabledPorts(int64_t mask);

    /**
     * @brief Enables specified safe I/O ports with a given configuration.
     *
     * @param mask Bitmask representing the safe I/O ports to enable.
     * @param config Configuration for the safe ports.
     * @return Reference to the modified RequestIO object.
     */
    RequestIO &withEnabledSafePorts(int64_t mask, ESafePortConfiguration config);
};

} // namespace kr2::kord

#endif // KR2_KORD_IO_REQUEST_H
