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

#ifndef KR2_KORD_CONTENT_FRAME_H
#define KR2_KORD_CONTENT_FRAME_H

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace kr2::kord::protocol {

/**
 * @brief Maximum number of content items that a content frame can contain.
 */
constexpr int MAX_CONTENT_ITEMS = 16;

/**
 * @brief Length of the KORD frame header in bytes.
 */
constexpr int KORD_FRAME_HDR_LEN = 16;

/**
 * @brief Maximum Ethernet data length in bytes, adjusted for MTU.
 *
 * This value is calculated as the remainder to the standard MTU of 1500 bytes.
 */
constexpr int MAX_ETH_DATA_LEN_B = 1458; // remainder to MTU 1500

/**
 * @brief Maximum UDP data length in bytes.
 *
 * Calculated by subtracting the KORD frame header length from the maximum Ethernet data length.
 */
constexpr int MAX_UDP_DATA_LEN = MAX_ETH_DATA_LEN_B - KORD_FRAME_HDR_LEN;

/**
 * @brief Maximum TCP data length in bytes.
 *
 * Set to the maximum value of a 16-bit unsigned integer.
 */
constexpr int MAX_TCP_DATA_LEN = 65535;

/**
 * @brief Current version of the KORD protocol.
 */
constexpr int KORD_PROTOCOL_VERSION = 1;

/**
 * @brief Initial version identifier for KORD status frames.
 */
constexpr int KORD_STATUS_VERSION_0 = 0;

/**
 * @brief Subsequent version identifier for KORD status frames.
 */
constexpr int KORD_STATUS_VERSION_1 = 1;

/**
 * @brief Latest version identifier for KORD status frames.
 */
constexpr int KORD_STATUS_VERSION_LATEST = KORD_STATUS_VERSION_1;

// enum { KORD_FRAME_ID_NEGOTIN = 1001 };
/**
 * @brief Frame ID for content frames in the KORD protocol.
 */
constexpr int KORD_FRAME_ID_CONTENT = 1002;

/**
 * @brief Frame ID for status frames in the KORD protocol.
 */
constexpr int KORD_FRAME_ID_STATUS = 1003;

/**
 * @brief Base template structure for KORD frames.
 *
 * This template defines the common header fields for KORD frames, parameterized by payload length.
 *
 * @tparam PAYLOAD_LEN Length of the payload in bytes.
 */
template <size_t PAYLOAD_LEN> struct KORDFrameBase {
    uint16_t frame_id_;            /**< @brief Identifier for the frame type. */
    uint16_t session_id_;          /**< @brief Identifier for the session. */
    int64_t tx_timestamp_;         /**< @brief Transmission timestamp in nanoseconds. */
    uint16_t kord_version_;        /**< @brief Version of the KORD protocol. */
    uint16_t payload_length_;      /**< @brief Length of the payload in bytes. */
    uint8_t payload_[PAYLOAD_LEN]; /**< @brief Payload data. */
} __attribute__((packed));

/**
 * @typedef KORDFrame
 * @brief UDP-specific KORD frame with predefined maximum payload length.
 */
using KORDFrame = KORDFrameBase<MAX_UDP_DATA_LEN>;

/**
 * @typedef KORDFrameTCP
 * @brief TCP-specific KORD frame with predefined maximum payload length.
 */
using KORDFrameTCP = KORDFrameBase<MAX_TCP_DATA_LEN>;

/**
 * @struct KORDContentHeader
 * @brief Header structure for individual content items within a content frame.
 */
struct KORDContentHeader {
    uint32_t item_id_;     /**< @brief Identifier for the content item. */
    uint16_t item_offset_; /**< @brief Offset of the item data within the frame. */
    uint16_t item_length_; /**< @brief Length of the item data in bytes. */

    /**
     * @brief Resets the content header to zero.
     */
    void reset() { memset(this, 0x00, sizeof(KORDContentHeader)); }

} __attribute__((packed));

/**
 * @brief Content frame is designed to carry up to 16 items, which may include
 * robot commands, robot requests, responses, or user-defined structures.
 *
 * @tparam DATA_LEN Length of the data section in bytes.
 */
template <size_t DATA_LEN> struct KORDContentFrameBase {
    uint16_t items_contained_; /**< @brief Number of content items contained. */
    uint8_t item_headers_[MAX_CONTENT_ITEMS * sizeof(KORDContentHeader)]; /**< @brief Array of content item headers. */
    uint8_t item_data_[DATA_LEN];                                         /**< @brief Data for the content items. */

    /**
     * @brief Retrieves the total length of the frame.
     *
     * @return Total length of the frame in bytes.
     */
    [[nodiscard]] size_t getFrameLength() const { return sizeof(KORDContentFrameBase<DATA_LEN>); }

    /**
     * @brief Retrieves the length of the data section.
     *
     * @return Length of the data section in bytes.
     */
    [[nodiscard]] size_t getDataLength() const { return sizeof(item_data_); }

    /**
     * @brief Resets the content frame to zero.
     */
    void reset() { memset(this, 0x00, sizeof(KORDContentFrameBase<DATA_LEN>)); }

} __attribute__((packed));

/**
 * @typedef KORDContentFrame
 * @brief UDP-specific content frame with predefined data length.
 */
using KORDContentFrame = KORDContentFrameBase<MAX_UDP_DATA_LEN - MAX_CONTENT_ITEMS * sizeof(KORDContentHeader) - 2>;

/**
 * @typedef KORDContentFrameTCP
 * @brief TCP-specific content frame with predefined data length.
 */
using KORDContentFrameTCP = KORDContentFrameBase<MAX_TCP_DATA_LEN - MAX_CONTENT_ITEMS * sizeof(KORDContentHeader) - 2>;

/**
 * @struct KORDStatusFrame
 * @brief Structure representing a status frame in the KORD protocol.
 */
struct KORDStatusFrame {
    uint16_t version_;                   /**< @brief Version of the status frame. */
    uint16_t frequency_;                 /**< @brief Frequency of status updates. */
    uint16_t sequence_number_;           /**< @brief Sequence number of the status frame. */
    uint8_t data_[MAX_UDP_DATA_LEN - 6]; /**< @brief Payload data for the status frame. */

    /**
     * @brief Retrieves the total length of the status frame.
     *
     * @return Total length of the status frame in bytes.
     */
    [[nodiscard]] size_t getFrameLength() const { return sizeof(KORDStatusFrame); }

    /**
     * @brief Retrieves the length of the data section in the status frame.
     *
     * @return Length of the data section in bytes.
     */
    [[nodiscard]] size_t getDataLength() const { return sizeof(data_); }

    /**
     * @brief Resets the status frame to zero.
     */
    void reset() { memset(this, 0x00, sizeof(KORDStatusFrame)); }

} __attribute__((packed));

} // namespace kr2::kord::protocol

#endif // KR2_KORD_CONTENT_FRAME_H
