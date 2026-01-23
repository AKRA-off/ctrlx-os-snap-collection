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

#ifndef KR2_KORD_SERVER_PARAMETERS_H
#define KR2_KORD_SERVER_PARAMETERS_H

#include <kord/protocol/ServerServiceIDs.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace kr2::kord::protocol {

/**
 * @class ServiceParameters
 * @brief Abstract base class for service parameters in the KORD protocol.
 *
 * This class serves as the base for all service parameter classes, providing a
 * common interface for dumping parameter data.
 */
class ServiceParameters {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~ServiceParameters() = default;

    /**
     * @brief Serializes the service parameters into a byte vector.
     *
     * @return A vector of bytes representing the serialized parameters.
     */
    virtual std::vector<uint8_t> dump() = 0;
};

/**
 * @class ServiceEmptyParameters
 * @brief Represents empty service parameters with no additional data.
 *
 * This class is used when a service does not require any parameters.
 */
class ServiceEmptyParameters : public ServiceParameters {
public:
    /**
     * @brief Default constructor.
     */
    ServiceEmptyParameters() = default;

    /**
     * @brief Serializes the service parameters.
     *
     * @return An empty vector since there are no parameters.
     */
    std::vector<uint8_t> dump() override { return {}; }
};

/**
 * @class ServiceFetchKincalParameters
 * @brief Represents parameters for the FetchKincalData service.
 *
 * This class encapsulates parameters specific to fetching Kincal data, such as
 * enabling tool I/O.
 */
class ServiceFetchKincalParameters : public ServiceParameters {
public:
    /**
     * @brief Default constructor that enables tool I/O by default.
     */
    ServiceFetchKincalParameters() : enable_tool_io_(true) {}

    /**
     * @brief Constructs the parameters with the specified tool I/O state.
     *
     * @param enable_tool_io Boolean flag to enable or disable tool I/O.
     */
    explicit ServiceFetchKincalParameters(bool enable_tool_io) : enable_tool_io_(enable_tool_io) {}

    /**
     * @brief Constructs the parameters from a byte vector.
     *
     * @param vec_ Byte vector containing serialized parameter data.
     */
    explicit ServiceFetchKincalParameters(const std::vector<uint8_t> &vec_)
    {
        if (!vec_.empty()) {
            enable_tool_io_ = static_cast<bool>(vec_[0]);
        }
    }

    /**
     * @brief Serializes the service parameters into a byte vector.
     *
     * @return A vector of bytes representing the serialized parameters.
     */
    std::vector<uint8_t> dump() override
    {
        std::vector<uint8_t> v(1);
        v[0] = static_cast<uint8_t>(enable_tool_io_);
        return v;
    }

    /**
     * @brief Checks whether tool I/O is enabled.
     *
     * @return True if tool I/O is enabled, false otherwise.
     */
    bool enable_tool_io_{};
};

/**
 * @class ServiceSoftwareUpdateParameters
 * @brief Represents parameters for the SoftwareUpdate service.
 *
 * This class encapsulates parameters required for performing a software update,
 * such as the file name of the update package.
 */
class ServiceSoftwareUpdateParameters : public ServiceParameters {
public:
    /**
     * @brief Constructs the parameters with an optional file name.
     *
     * @param file_name Name of the software update file. Defaults to an empty string.
     */
    explicit ServiceSoftwareUpdateParameters(const std::string &file_name = "") : file_name_(file_name) {}

    /**
     * @brief Constructs the parameters from a byte vector.
     *
     * @param vec_ Byte vector containing serialized parameter data.
     */
    explicit ServiceSoftwareUpdateParameters(const std::vector<uint8_t> &vec_)
    {
        uint16_t size = 0;
        if (!vec_.empty()) {
            size = static_cast<uint16_t>(vec_[0]) | (static_cast<uint16_t>(vec_[1]) << 8);
        }
        if (size > 0 && vec_.size() >= 2 + size) {
            file_name_ = std::string(vec_.begin() + 2, vec_.begin() + 2 + size);
        }
    }

    /**
     * @brief Serializes the service parameters into a byte vector.
     *
     * @return A vector of bytes representing the serialized parameters.
     */
    std::vector<uint8_t> dump() override
    {
        auto size = static_cast<uint16_t>(file_name_.size());
        std::vector<uint8_t> v(2 + size);
        v[0] = size & 0xFF;
        v[1] = (size >> 8) & 0xFF;
        if (size > 0) {
            std::copy(file_name_.begin(), file_name_.end(), v.begin() + 2);
        }
        return v;
    }

    /**
     * @brief Retrieves the file name for the software update.
     *
     * @return A constant reference to the file name string.
     */
    [[nodiscard]] const std::string &getFileName() const { return file_name_; }

private:
    /**
     * @brief Name of the software update file.
     */
    std::string file_name_;
};

/**
 * @brief Mapping from server service IDs to their corresponding parameter constructors.
 *
 * This map associates each EKORDServerServiceID with a factory function that creates
 * the appropriate ServiceParameters object based on the provided byte vector.
 */
inline std::map<EKORDServerServiceID, std::function<std::shared_ptr<ServiceParameters>(const std::vector<uint8_t> &)>>
    ServiceID2Parameters{
        {EKORDServerServiceID::eFetchKincalData,
         [](const std::vector<uint8_t> &vec_) { return std::make_shared<ServiceFetchKincalParameters>(vec_); }},
        {EKORDServerServiceID::eSoftwareUpdate,
         [](const std::vector<uint8_t> &) { return std::make_shared<ServiceEmptyParameters>(); }},
    };

} // namespace kr2::kord::protocol

#endif // KR2_KORD_SERVER_PARAMETERS_H
