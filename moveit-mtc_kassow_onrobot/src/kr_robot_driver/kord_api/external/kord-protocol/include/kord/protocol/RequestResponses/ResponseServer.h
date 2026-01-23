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

#ifndef KR2_KORD_PROTOCOL_SERVER_RESPONSE_H
#define KR2_KORD_PROTOCOL_SERVER_RESPONSE_H

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
#include <kord/protocol/RequestResponses/Response.h>
#include <kord/protocol/ServerServiceStatus.h>
#include <kord/protocol/StringUtils.h>

namespace kr2::kord::protocol {

/**
 * @typedef token_t
 * @brief Alias for a 64-bit integer token type.
 */
using token_t = int64_t;

/**
 * @class ServerResponse
 * @brief Generic response class for server requests.
 *
 * The ServerResponse class serves as a generic response handler for various server requests.
 * It encapsulates the status and progress of a server operation, providing methods to access
 * these details. This class inherits from the Response base class and extends its functionality
 * to handle specific response data relevant to server interactions.
 */
class ServerResponse : public Response {
public:
    /**
     * @brief Constructs a ServerResponse object by parsing the raw response data.
     *
     * This constructor initializes the ServerResponse object by loading and parsing the
     * raw byte vector containing the response data. It assumes that the data represents
     * a valid server response with a payload length of exactly 2 bytes.
     *
     * @param vec The raw byte vector containing the response data.
     */
    explicit ServerResponse(const std::vector<uint8_t> &vec) : Response(vec) { ServerResponse::load(vec, true); }

    /**
     * @brief Constructs a ServerResponse object with specified status and progress.
     *
     * This constructor allows the creation of a ServerResponse object by directly specifying
     * the status and progress values. It is useful for scenarios where the response data
     * is generated programmatically rather than received from a server.
     *
     * @param status The status code of the server response.
     * @param progress The progress indicator of the server response.
     */
    explicit ServerResponse(uint8_t status, uint8_t progress) : progress_(progress), status_(status) {}

    /**
     * @brief Destructor for ServerResponse.
     *
     * The destructor ensures proper cleanup of resources. Since no dynamic memory allocation
     * is performed within the ServerResponse class, the default destructor is sufficient.
     */
    ~ServerResponse() override = default;

    /**
     * @brief Dumps the server response data into a byte vector.
     *
     * This method serializes the server response data, specifically the status and progress
     * bytes, into a byte vector. This serialized data can then be transmitted or stored as needed.
     *
     * @return A vector containing the serialized status and progress bytes.
     */
    [[nodiscard]] std::vector<uint8_t> dump() override
    {
        std::vector<uint8_t> data(2);
        data[0] = status_;
        data[1] = progress_;
        return data;
    }

    /**
     * @brief Loads and parses the server response data from the provided byte vector.
     *
     * This method overrides the base class's load method to specifically handle server
     * response data. It expects the payload to be exactly 2 bytes long, representing the
     * status and progress of the server operation. If the payload length is invalid, an
     * error message is printed to the standard error stream.
     *
     * @param data The byte vector containing the server response data.
     * @param is_payload_vector Flag indicating if the data is a payload vector.
     * @return Reference to the current ServerResponse object.
     */
    ServerResponse &load(const std::vector<uint8_t> &data, bool is_payload_vector) override
    {
        Response::load(data, is_payload_vector);
        if (payload_length_ != 2) {
            std::cerr << "ServerResponse: Invalid payload length: " << payload_length_ << std::endl;
            return *this;
        }
        status_ = payload_[0];
        progress_ = payload_[1];
        return *this;
    }

    /**
     * @brief Retrieves the progress indicator of the server response.
     *
     * @return The progress value as an unsigned 8-bit integer (0-100).
     */
    [[nodiscard]] uint8_t getProgress() const { return progress_; }

    /**
     * @brief Retrieves the status code of the server response.
     *
     * @return The status value as an unsigned 8-bit integer.
     */
    [[nodiscard]] uint8_t getStatus() const { return status_; }

    /**
     * @brief Checks if the server response indicates a successful operation.
     *
     * @return `true` if the status corresponds to a successful operation, `false` otherwise.
     */
    [[nodiscard]] bool isSuccess() const { return status_ == static_cast<uint8_t>(EServiceStatus::eSuccess); }

    /**
     * @brief Checks if the server response indicates a failed operation.
     *
     * @return `true` if the status corresponds to a failed operation, `false` otherwise.
     */
    [[nodiscard]] bool isFailed() const { return status_ == static_cast<uint8_t>(EServiceStatus::eFailed); }

    /**
     * @brief Checks if the server response indicates that an operation is in progress.
     *
     * @return `true` if the status corresponds to an operation in progress, `false` otherwise.
     */
    [[nodiscard]] bool isInProgress() const { return status_ == static_cast<uint8_t>(EServiceStatus::eProgress); }

    /**
     * @brief Checks if the server response indicates that the server is idle, i.e., no operation is in progress.
     * Usually, this status is returned if the server did not receive any requests.
     *
     * @return `true` if the status corresponds to an idle server state, `false` otherwise.
     */
    [[nodiscard]] bool isIdle() const { return status_ == static_cast<uint8_t>(EServiceStatus::eIdle); }

protected:
    uint8_t progress_{}; /**< The progress indicator of the server response (0-100) */
    uint8_t status_{};   /**< The status code of the server response.
                             Values are defined in the EServiceStatus enum. */
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_PROTOCOL_SERVER_RESPONSE_H
