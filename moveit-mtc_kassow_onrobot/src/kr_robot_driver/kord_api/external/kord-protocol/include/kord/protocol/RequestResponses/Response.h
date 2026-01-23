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

#ifndef KR2_KORD_PROTOCOL_RESPONSE_H
#define KR2_KORD_PROTOCOL_RESPONSE_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <kord/protocol/DataDescriptions/ResponsesDFD.h>
#include <kord/protocol/StringUtils.h>

namespace kr2::kord::protocol {

/**
 * @typedef token_t
 * @brief Alias for a 64-bit integer token type.
 *
 * Used to uniquely identify responses within the KORD protocol.
 */
using token_t = int64_t;

/**
 * @brief Data format description for responses.
 *
 * This static variable holds the data format description for the latest response item,
 * identified by EKORDItemID::eResponse. It is used to parse and serialize response data.
 */
static auto dfd = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eResponse);

/**
 * @class Response
 * @brief Generic response base class for all responses in the KORD protocol.
 *
 * The Response class serves as a generic handler for various types of responses received
 * from the server. All specific response classes should inherit from this base class and
 * implement their own load and dump methods as required.
 */
class Response {
public:
    /**
     * @brief Default constructor for the Response class.
     *
     * Initializes an empty Response object without any data.
     */
    Response() = default;

    /**
     * @brief Virtual destructor for the Response class.
     *
     * Ensures proper cleanup of derived classes when deleting objects through base class pointers.
     */
    virtual ~Response() = default;

    /**
     * @brief Constructs a Response object by parsing the raw response data.
     *
     * This constructor initializes the Response object by loading and parsing the
     * provided byte vector containing the response data.
     *
     * @param vec The raw byte vector containing the response data.
     */
    explicit Response(const std::vector<uint8_t> &vec) { Response::load(vec); }

    /**
     * @brief Serializes the response data into a vector of bytes.
     *
     * This virtual method should be overridden by derived classes to provide
     * serialization of response-specific data. The base implementation returns an
     * empty vector and should be customized as needed.
     *
     * @return A vector of bytes representing the serialized response data.
     */
    [[nodiscard]] virtual std::vector<uint8_t> dump()
    {
        std::vector<uint8_t> data;
        // TODO: Implement serialization in derived classes
        return data;
    }

    /**
     * @brief Loads and parses the response data from a serialized vector of bytes.
     *
     * This method parses the provided byte vector to extract the response token, payload length,
     * payload data, and response code. It uses the data format description (dfd) to determine
     * the offsets of each piece of information within the data.
     *
     * @param data The byte vector containing the serialized response data.
     * @param is_payload_vector Flag indicating if the data is a payload vector, affecting the payload offset.
     * @return Reference to the current Response object.
     */
    virtual Response &load(const std::vector<uint8_t> &data, bool is_payload_vector = false)
    {
        if (data.empty()) {
            std::cerr << "Response: empty data" << std::endl;
            raw_data_.clear();
            return *this;
        }

        raw_data_ = data;

        // Extract the response token
        unsigned int offset = dfd.getOffset(EKORDDataID::eResponseToken);
        std::memcpy(&token_, data.data() + offset, sizeof(token_));

        // Extract the payload length
        offset = dfd.getOffset(EKORDDataID::eResponsePayloadLength);
        std::memcpy(&payload_length_, data.data() + offset, sizeof(payload_length_));

        // If there is no payload, return early
        if (payload_length_ == 0) {
            return *this;
        }

        // Resize the payload vector to hold the payload data
        payload_.resize(payload_length_);

        // Calculate the extra offset based on whether the data is a payload vector
        int extra_offset = (is_payload_vector) ? 2 : 0;

        // Extract the payload data
        offset = dfd.getOffset(EKORDDataID::eResponsePayload) + extra_offset;
        std::memcpy(payload_.data(), data.data() + offset, payload_length_);

        // Extract the response code
        offset = dfd.getOffset(EKORDDataID::eResponseCode);
        std::memcpy(&code_, data.data() + offset, sizeof(code_));

        return *this;
    }

    /**
     * @brief Retrieves the raw serialized response data.
     *
     * @return A vector of bytes containing the raw response data.
     */
    [[nodiscard]] std::vector<uint8_t> getRawData() const { return raw_data_; }

    /**
     * @brief Retrieves the unique token associated with the response.
     *
     * @return The response token as a 64-bit integer.
     */
    [[nodiscard]] token_t getToken() const { return token_; }

    /**
     * @brief Retrieves the response code.
     *
     * @return The response code as an unsigned 16-bit integer.
     */
    [[nodiscard]] uint16_t getCode() const { return code_; }

    /**
     * @brief Retrieves the payload data of the response.
     *
     * @return A vector of bytes containing the payload data.
     */
    [[nodiscard]] std::vector<uint8_t> getPayload() const { return payload_; }

    /**
     * @brief Retrieves the length of the payload data.
     *
     * @return The payload length as an unsigned 16-bit integer.
     */
    [[nodiscard]] uint16_t getPayloadLength() const { return payload_length_; }

protected:
    std::vector<uint8_t> raw_data_{}; /**< The raw serialized response data */

    token_t token_{};                /**< The unique token identifying the response */
    uint16_t code_{};                /**< The response code indicating the result of the request */
    std::vector<uint8_t> payload_{}; /**< The payload data associated with the response */
    uint16_t payload_length_{};      /**< The length of the payload data in bytes */
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_PROTOCOL_RESPONSE_H
