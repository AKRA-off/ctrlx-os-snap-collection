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

#ifndef KR2_KORD_PROTOCOL_GET_SAFETY_ZONES_RESPONSE_H
#define KR2_KORD_PROTOCOL_GET_SAFETY_ZONES_RESPONSE_H

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
#include <kord/protocol/RequestResponses/Response.h>
#include <kord/protocol/RequestResponses/Types.h>

namespace kr2::kord::protocol {

/**
 * @typedef token_t
 * @brief Alias for a 64-bit integer token type.
 */
using token_t = int64_t;

/**
 * @class GetSafetyZonesResponse
 * @brief Handles responses for retrieving safety zones from the robot.
 *
 * This class parses and manages the safety zones data received from the KORD.
 * It provides methods to access individual safety zones, retrieve
 * all zone DUIDs, and output the safety zones information.
 */
class GetSafetyZonesResponse : public Response {
public:
    /**
     * @brief Maximum number of safety zones supported.
     */
    constexpr static size_t MAX_ZONES = 16;

    /**
     * @brief Constructs a GetSafetyZonesResponse object by parsing the raw response data.
     *
     * @param raw_response The raw byte vector containing the response data.
     */
    explicit GetSafetyZonesResponse(const std::vector<uint8_t> &raw_response) : Response(raw_response)
    {
        GetSafetyZonesResponse::load(raw_response, false);
    }

    /**
     * @brief Loads and parses the safety zones data from the provided byte vector.
     *
     * @param data The byte vector containing the safety zones data.
     * @param is_payload_vector Flag indicating if the data is a payload vector.
     * @return Reference to the current GetSafetyZonesResponse object.
     */
    GetSafetyZonesResponse &load(const std::vector<uint8_t> &data, bool is_payload_vector) override
    {
        Response::load(data, is_payload_vector);
        std::memcpy(safety_zones_.data(), payload_.data(), payload_length_);
        return *this;
    }

    /**
     * @brief Overloads the output stream operator to print safety zones information.
     *
     * @param os The output stream.
     * @param response The GetSafetyZonesResponse object to be printed.
     * @return Reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &os, const GetSafetyZonesResponse &response)
    {
        os << "Safety zones: " << std::endl;
        for (const auto &zone : response.getSafetyZones()) {
            if (zone.zone_duid_ == 0)
                continue;
            os << "  - Zone DUID: " << zone.zone_duid_ << ", name: " << std::string(zone.zone_label_) << "\n";
            auto n_geoms = zone.n_geometries_;
            for (size_t i = 0; i < n_geoms; ++i) {
                auto geometry = zone.geometries_[i];
                double points[3], normals[3];
                std::memcpy(points, geometry.points_, sizeof(points));
                std::memcpy(normals, geometry.normals_, sizeof(normals));
                os << "\t> Geometry: points=(" << points[0] << ", " << points[1] << ", " << points[2] << "), "
                   << "(normals=" << normals[0] << ", " << normals[1] << ", " << normals[2] << ")\n";
            }
        }

        return os;
    }

    /**
     * @brief Retrieves a list of all safety zone DUIDs.
     *
     * @return A vector containing the DUIDs of all valid safety zones.
     */
    [[nodiscard]] std::vector<uint32_t> getSafetyZonesDUIDs() const
    {
        std::vector<uint32_t> duids;
        for (const auto &zone : safety_zones_) {
            if (zone.zone_duid_ == 0) {
                continue;
            }
            duids.push_back(zone.zone_duid_);
        }
        return duids;
    }

    /**
     * @brief Retrieves a specific safety zone by its DUID.
     *
     * @param duid The DUID of the safety zone to retrieve.
     * @return The KORDSafetyZone object corresponding to the provided DUID.
     * @throws std::runtime_error if the safety zone with the specified DUID is not found.
     */
    [[nodiscard]] KORDSafetyZone getSafetyZone(uint32_t duid) const
    {
        auto it = std::find_if(safety_zones_.begin(), safety_zones_.end(), [duid](const KORDSafetyZone &zone) {
            return zone.zone_duid_ == duid;
        });
        if (it != safety_zones_.end()) {
            return *it;
        }
        throw std::runtime_error("Safety zone not found");
    }

    /**
     * @brief Retrieves all safety zones.
     *
     * @return A constant reference to the array containing all safety zones.
     */
    [[nodiscard]] const std::array<KORDSafetyZone, MAX_ZONES> &getSafetyZones() const { return safety_zones_; }

    // /**
    //  * @brief Dumps the raw response data.
    //  *
    //  * @return A vector containing the raw byte data.
    //  */
    // [[nodiscard]] std::vector<uint8_t> dump() override { return raw_data_; }

private:
    /**
     * @brief Array storing the safety zones.
     *
     * Each safety zone contains information such as DUID, label, and geometries.
     */
    std::array<KORDSafetyZone, MAX_ZONES> safety_zones_{};
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_PROTOCOL_GET_SAFETY_ZONES_RESPONSE_H
