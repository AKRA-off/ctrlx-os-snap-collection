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

#ifndef KR2_KORD_PROTOCOL_GET_VERSION_RESPONSE_H
#define KR2_KORD_PROTOCOL_GET_VERSION_RESPONSE_H

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
 * @class GetVersionResponse
 * @brief Handles responses for retrieving version information from the robot.
 *
 * This class parses and manages the version information data received from the KORD.
 * It provides methods to access specific version details of various
 * components such as the server, controllers, MCUs, and more.
 */
class GetVersionResponse : public Response {
public:
    /**
     * @brief Constructs a GetVersionResponse object by parsing the raw response data.
     *
     * @param vec The raw byte vector containing the response data.
     */
    explicit GetVersionResponse(const std::vector<uint8_t> &vec) : Response(vec) { GetVersionResponse::load(vec, true); }

    /**
     * @brief Constructs a GetVersionResponse object from a raw versions string.
     *
     * @param raw_versions The raw versions string.
     */
    explicit GetVersionResponse(const std::string &raw_versions) : raw_versions_(raw_versions) {}

    /**
     * @struct VersionInfo
     * @brief Structure to hold version information of a component.
     */
    struct VersionInfo {
        /**
         * @brief Constructs a VersionInfo object.
         *
         * @param component_name The name of the component.
         * @param version The version string of the component.
         * @param commit_hash The commit hash of the component's source.
         */
        VersionInfo(const std::string &component_name, const std::string &version, const std::string &commit_hash)
            : component_name(component_name), version(version), commit_hash(commit_hash)
        {
        }

        std::string component_name; /**< The name of the component */
        std::string version;        /**< The version string of the component */
        std::string commit_hash;    /**< The commit hash of the component's source */
    };

    /**
     * @enum VersionType
     * @brief Enumerates the types of version information available.
     * @note At this moment, MCU B is equivalent to MCU A in all contexts.
     */
    enum class VersionType : uint8_t {
        SERVER,                /**< Server version */
        RESPONSIVE_CONTROLLER, /**< Responsive controller version */
        KORD_CBUN,             /**< KORD CBUN version */
        KORD_API,              /**< KORD API version */
        KORD_PROTOCOL,         /**< KORD Protocol version */
        JB_1_MCU_A,            /**< JB 1 MCU A version */
        JB_1_MCU_B,            /**< JB 1 MCU B version */
        JB_2_MCU_A,            /**< JB 2 MCU A version */
        JB_2_MCU_B,            /**< JB 2 MCU B version */
        JB_3_MCU_A,            /**< JB 3 MCU A version */
        JB_3_MCU_B,            /**< JB 3 MCU B version */
        JB_4_MCU_A,            /**< JB 4 MCU A version */
        JB_4_MCU_B,            /**< JB 4 MCU B version */
        JB_5_MCU_A,            /**< JB 5 MCU A version */
        JB_5_MCU_B,            /**< JB 5 MCU B version */
        JB_6_MCU_A,            /**< JB 6 MCU A version */
        JB_6_MCU_B,            /**< JB 6 MCU B version */
        JB_7_MCU_A,            /**< JB 7 MCU A version */
        JB_7_MCU_B,            /**< JB 7 MCU B version */
        IOB_HW_MCU_A,          /**< IO Board Hardware MCU A version */
        IOB_HW_MCU_B,          /**< IO Board Hardware MCU B version */
        IOB_FW_MCU_A,          /**< IO Board Firmware MCU A version */
        IOB_FW_MCU_B,          /**< IO Board Firmware MCU B version */
        TB_FW_MCU_A,           /**< Tablet Firmware MCU A version */
        TB_FW_MCU_B,           /**< Tablet Firmware MCU B version */
        TB_HW_MCU_A,           /**< Tablet Hardware MCU A version */
        TB_HW_MCU_B,           /**< Tablet Hardware MCU B version */
        OTHER                  /**< Any other version type, not explicitly defined */
    };

    const std::unordered_map<std::string, VersionType> string2version{
        {"kr2_server", VersionType::SERVER},           {"kr2_responsive_controller", VersionType::RESPONSIVE_CONTROLLER},
        {"kord_cbun", VersionType::KORD_CBUN},         {"kord_api", VersionType::KORD_API},
        {"kord_protocol", VersionType::KORD_PROTOCOL}, {"jb_1_fw_mcu_a", VersionType::JB_1_MCU_A},
        {"jb_1_fw_mcu_b", VersionType::JB_1_MCU_B},    {"jb_2_fw_mcu_a", VersionType::JB_2_MCU_A},
        {"jb_2_fw_mcu_b", VersionType::JB_2_MCU_B},    {"jb_3_fw_mcu_a", VersionType::JB_3_MCU_A},
        {"jb_3_fw_mcu_b", VersionType::JB_3_MCU_B},    {"jb_4_fw_mcu_a", VersionType::JB_4_MCU_A},
        {"jb_4_fw_mcu_b", VersionType::JB_4_MCU_B},    {"jb_5_fw_mcu_a", VersionType::JB_5_MCU_A},
        {"jb_5_fw_mcu_b", VersionType::JB_5_MCU_B},    {"jb_6_fw_mcu_a", VersionType::JB_6_MCU_A},
        {"jb_6_fw_mcu_b", VersionType::JB_6_MCU_B},    {"jb_7_fw_mcu_a", VersionType::JB_7_MCU_A},
        {"jb_7_fw_mcu_b", VersionType::JB_7_MCU_B},    {"ioboard_hw", VersionType::IOB_HW_MCU_A},
        {"ioboard_fw", VersionType::IOB_FW_MCU_A},     {"toolio_fw", VersionType::TB_FW_MCU_A},
        {"toolio_hw", VersionType::TB_HW_MCU_A},
    };

    /**
     * @brief Retrieves version information for a specific VersionType.
     *
     * @param type The VersionType for which to retrieve the version information.
     * @return A constant reference to the corresponding VersionInfo object.
     * @throws std::runtime_error if the version information for the specified type is not found.
     */
    [[nodiscard]] const VersionInfo &getVersionInfo(VersionType type) const
    {
        auto it = version_map_.find(type);
        if (it != version_map_.end()) {
            return it->second;
        }
        throw std::runtime_error("Version not found");
    }

    /**
     * @brief Retrieves all version information as a sorted vector.
     *
     * @return A vector containing all VersionInfo objects, sorted by component name.
     */
    [[nodiscard]] std::vector<VersionInfo> getAllVersionsInfo() const
    {
        std::vector<VersionInfo> all_versions;
        all_versions.reserve(version_map_.size());
        for (const auto &it : version_map_) {
            all_versions.emplace_back(it.second);
        }
        std::sort(all_versions.begin(), all_versions.end(), [](const VersionInfo &a, const VersionInfo &b) {
            return a.component_name < b.component_name;
        });
        return all_versions;
    }

    /**
     * @brief Overloads the output stream operator to print all version information.
     *
     * @param os The output stream.
     * @param response The GetVersionResponse object to be printed.
     * @return Reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &os, const GetVersionResponse &response)
    {
        auto versions = response.getAllVersionsInfo();
        os << versions;
        return os;
    }

    /**
     * @brief Overloads the output stream operator to print a single VersionInfo object.
     *
     * @param os The output stream.
     * @param info The VersionInfo object to be printed.
     * @return Reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &os, const VersionInfo &info)
    {
        os << "Component: " << info.component_name << ", version: " << (info.version.empty() ? "N/A" : info.version)
           << ", commit hash: " << (info.commit_hash.empty() ? "N/A" : info.commit_hash);
        return os;
    }

    /**
     * @brief Overloads the output stream operator to print a vector of VersionInfo objects.
     *
     * @param os The output stream.
     * @param versions The vector of VersionInfo objects to be printed.
     * @return Reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &os, std::vector<VersionInfo> &versions)
    {
        for (const auto &version : versions) {
            os << version << std::endl;
        }
        return os;
    }

    /**
     * @brief Retrieves the raw versions string.
     *
     * @return The raw versions as a string.
     */
    [[nodiscard]] std::string getRawVersions() const { return raw_versions_; }

    /**
     * @brief Loads and parses the version data from the provided byte vector.
     *
     * @param data The byte vector containing the version data.
     * @param is_payload_vector Flag indicating if the data is a payload vector.
     * @return Reference to the current GetVersionResponse object.
     */
    Response &load(const std::vector<uint8_t> &data, bool is_payload_vector) override
    {
        Response::load(data, is_payload_vector);
        // Move payload 2 bytes to the right to skip payload length
        raw_versions_ = std::string(payload_.begin(), payload_.end());
        parseVersions();
        return *this;
    }

    /**
     * @brief Dumps the raw versions data into a byte vector.
     *
     * @return A vector containing the raw versions as bytes.
     */
    [[nodiscard]] std::vector<uint8_t> dump() override { return {raw_versions_.begin(), raw_versions_.end()}; }

private:
    /**
     * @brief Internal map to store version data for quick lookup.
     *
     * The key is of type VersionType, and the value is the corresponding VersionInfo object.
     */
    std::multimap<VersionType, VersionInfo> version_map_{};

    /**
     * @brief The raw versions string received from the server.
     */
    std::string raw_versions_{};

    /**
     * @brief Trims whitespace from both ends of a string.
     *
     * @param str The string to trim.
     * @return A new string with leading and trailing whitespace removed.
     */
    static std::string trim(const std::string &str)
    {
        const std::string whitespace = " \t\n\r";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos)
            return ""; // All whitespace

        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    /**
     * @brief Splits a string into a vector of substrings based on a delimiter.
     *
     * @param s The string to split.
     * @param delimiter The character delimiter to use for splitting.
     * @return A vector of substrings.
     */
    static std::vector<std::string> split(const std::string &s, char delimiter)
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);

        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }

        return tokens;
    }

    /**
     * @brief Parses the raw versions string and populates the version_map_.
     *
     * This method processes the raw_versions_ string, splitting it into lines and fields,
     * and populates the version_map_ with the corresponding VersionType and VersionInfo.
     *
     * Invalid lines or fields with insufficient data are skipped with an error message.
     */
    void parseVersions()
    {
        version_map_.clear();
        std::istringstream stream(raw_versions_);
        std::string line;

        while (std::getline(stream, line)) {
            if (line.empty()) {
                continue;
            }

            std::vector<std::string> fields = split(line, ';');
            if (fields.size() < 2) {
                KORD_LOG_ERROR("Invalid line (less than 2 fields): " << line);
                continue;
            }

            std::string name = trim(fields[0]);
            std::string version = trim(fields[1]);
            std::string hash = (fields.size() >= 3) ? trim(fields[2]) : "";

            VersionType key = VersionType::OTHER;
            if (auto it = string2version.find(name); it != string2version.end()) {
                key = it->second;
            }
            version_map_.emplace(key, VersionInfo(name, version, hash));
        }
    }
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_PROTOCOL_GET_VERSION_RESPONSE_H
