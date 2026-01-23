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

#ifndef KR2_KORD_PROTOCOL_SOFTWARE_UPDATE_RESPONSE_H
#define KR2_KORD_PROTOCOL_SOFTWARE_UPDATE_RESPONSE_H

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
#include <kord/protocol/RequestResponses/ResponseServer.h>
#include <kord/protocol/ServerServiceStatus.h>
#include <kord/protocol/StringUtils.h>

namespace kr2::kord::protocol {

/**
 * @typedef token_t
 * @brief Alias for a 64-bit integer token type.
 */
using token_t = int64_t;

/**
 * @class SoftwareUpdateResponse
 * @brief Response class for the RequestSystem.asSoftwareUpdate() request.
 *
 * Note: This functionality is not yet available in KORD.
 *
 * The SoftwareUpdateResponse class handles responses related to software update requests.
 * It inherits from the ServerResponse class, utilizing its
 * functionality to manage status and progress indicators specific to software updates.
 */
class SoftwareUpdateResponse : public ServerResponse {
public:
    /**
     * @brief Constructs a SoftwareUpdateResponse object by parsing the raw response data.
     *
     * This constructor initializes the SoftwareUpdateResponse object by loading and parsing the
     * raw byte vector containing the response data. It assumes that the data represents a valid
     * software update response.
     *
     * @param vec The raw byte vector containing the response data.
     */
    explicit SoftwareUpdateResponse(const std::vector<uint8_t> &vec) : ServerResponse(vec)
    {
        SoftwareUpdateResponse::load(vec, true);
    }

    /**
     * @brief Loads and parses the software update response data from the provided byte vector.
     *
     * This method overrides the base class's load method to handle software update-specific response data.
     * It utilizes the ServerResponse's load method to parse the status and progress indicators.
     *
     * @param data The byte vector containing the software update response data.
     * @param is_payload_vector Flag indicating if the data is a payload vector.
     * @return Reference to the current SoftwareUpdateResponse object.
     */
    SoftwareUpdateResponse &load(const std::vector<uint8_t> &data, bool is_payload_vector) override
    {
        ServerResponse::load(data, is_payload_vector);
        return *this;
    }

    // /**
    //  * @brief Dumps the software update response data into a byte vector.
    //  *
    //  * This method serializes the software update response data into a byte vector for transmission or storage.
    //  *
    //  * @return A vector containing the serialized software update response bytes.
    //  */
    // [[nodiscard]] std::vector<uint8_t> dump() override { return raw_data_; }
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_PROTOCOL_SOFTWARE_UPDATE_RESPONSE_H
