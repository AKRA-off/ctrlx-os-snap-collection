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

#ifndef KR2_KORD_PROTOCOL_GET_ROBOT_INFO_RESPONSE_H
#define KR2_KORD_PROTOCOL_GET_ROBOT_INFO_RESPONSE_H

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
#include <kord/protocol/StringUtils.h>

namespace kr2::kord::protocol {

/**
 * @typedef token_t
 * @brief Alias for a 64-bit integer token type.
 */
using token_t = int64_t;

/**
 * @class GetRobotInfoResponse
 * @brief Handles responses for retrieving robot information from the robot.
 *
 * This class parses and manages the robot information data received from the KORD.
 * It provides methods to access specific pieces of robot information
 * such as model, serial numbers, and tool IO status.
 */
class GetRobotInfoResponse : public Response {
public:
    /**
     * @brief Constructs a GetRobotInfoResponse object by parsing the raw response data.
     *
     * @param vec The raw byte vector containing the response data.
     */
    explicit GetRobotInfoResponse(const std::vector<uint8_t> &vec) : Response(vec)
    {
        GetRobotInfoResponse::load(vec, true);
        parseVersions();
    }

    /**
     * @enum RobotInfoType
     * @brief Enumerates the types of robot information available.
     */
    enum class RobotInfoType : uint8_t {
        ROBOT_MODEL,    /**< The model of the robot */
        TOOL_IO,        /**< The Tool IO status */
        MANIPULATOR_SN, /**< The manipulator's serial number */
        CONTROLLER_SN,  /**< The controller's serial number */
        TABLET_SN,      /**< The tablet's serial number */
        OTHER           /**< Any other type of robot information */
    };

    const std::unordered_map<std::string, RobotInfoType> string2version{
        {"robot_model", RobotInfoType::ROBOT_MODEL},
        {"tool_io", RobotInfoType::TOOL_IO},
        {"manipulator_sn", RobotInfoType::MANIPULATOR_SN},
        {"controller_sn", RobotInfoType::CONTROLLER_SN},
        {"tablet_sn", RobotInfoType::TABLET_SN},
    };

    /**
     * @brief Loads and parses the robot information data from the provided byte vector.
     *
     * @param data The byte vector containing the robot information data.
     * @param is_payload_vector Flag indicating if the data is a payload vector.
     * @return Reference to the current GetRobotInfoResponse object.
     */
    Response &load(const std::vector<uint8_t> &data, bool is_payload_vector) override
    {
        Response::load(data, is_payload_vector);
        raw_info_ = std::string(payload_.begin(), payload_.end());
        return *this;
    }

    /**
     * @brief Parses the raw robot information into a map for easy access.
     *
     * This method processes the raw_info_ string, splitting it into lines and fields,
     * and populates the info_map_ with the corresponding RobotInfoType and values.
     *
     * Invalid lines or fields with insufficient data are skipped with an error message.
     */
    void parseVersions()
    {
        info_map_.clear();
        std::istringstream stream(raw_info_);
        std::string line;

        while (std::getline(stream, line)) {
            if (line.empty()) {
                continue;
            }

            std::vector<std::string> fields = StringUtils::split(line, ';');
            if (fields.size() < 2) {
                continue;
            }

            std::string name = StringUtils::trim(fields[0]);
            std::string value = StringUtils::trim(fields[1]);

            RobotInfoType key = RobotInfoType::OTHER;
            if (auto it = string2version.find(name); it != string2version.end()) {
                key = it->second;
            }

            info_map_.emplace(key, value);
        }
    }

    /**
     * @brief Retrieves specific robot information based on the provided type.
     *
     * @param type The type of robot information to retrieve.
     * @return The corresponding robot information as a string.
     *         Returns empty string if the information is not available.
     */
    std::string getRobotInfo(RobotInfoType type) const
    {
        auto it = info_map_.find(type);
        if (it != info_map_.end()) {
            return it->second;
        }
        return "";
    }

    /**
     * @brief Retrieves the robot model information.
     *
     * @return The robot model as a string.
     */
    std::string getRobotModel() const { return getRobotInfo(RobotInfoType::ROBOT_MODEL); }

    /**
     * @brief Retrieves the manipulator's serial number.
     *
     * @return The manipulator's serial number as a string.
     */
    std::string getManipulatorSN() const { return getRobotInfo(RobotInfoType::MANIPULATOR_SN); }

    /**
     * @brief Retrieves the controller's serial number.
     *
     * @return The controller's serial number as a string.
     */
    std::string getControllerSN() const { return getRobotInfo(RobotInfoType::CONTROLLER_SN); }

    /**
     * @brief Retrieves the tablet's serial number.
     *
     * @return The tablet's serial number as a string.
     */
    std::string getTabletSN() const { return getRobotInfo(RobotInfoType::TABLET_SN); }

    /**
     * @brief Retrieves the Tool IO status.
     *
     * @return The Tool IO status as a boolean.
     *         Returns false if the status cannot be parsed.
     */
    bool getToolIO() const
    {
        bool res;
        std::istringstream(getRobotInfo(RobotInfoType::TOOL_IO)) >> std::boolalpha >> res;
        return res;
    }

    /**
     * @brief Retrieves the raw robot information string.
     *
     * @return The raw robot information as a string.
     */
    [[nodiscard]] std::string getRawInfo() const { return raw_info_; }

private:
    /**
     * @brief The raw robot information string received from the response.
     */
    std::string raw_info_;

    /**
     * @brief A map storing the parsed robot information.
     *
     * The key is of type RobotInfoType, and the value is the corresponding information as a string.
     */
    std::map<RobotInfoType, std::string> info_map_;
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_PROTOCOL_GET_ROBOT_INFO_RESPONSE_H
