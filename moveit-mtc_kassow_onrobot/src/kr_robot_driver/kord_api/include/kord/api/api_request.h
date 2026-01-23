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

#ifndef KR2_KORD_API_REQUEST_H
#define KR2_KORD_API_REQUEST_H

#pragma once

#include <kord/protocol/DataDescriptions/Requests/ControlCommandItems.h>
#include <kord/protocol/DataDescriptions/Requests/ControlCommandStatus.h>
#include <kord/protocol/DataDescriptions/Requests/RCAPICommandItems.h>
#include <kord/protocol/DataDescriptions/Requests/ServerServiceCommands.h>
#include <kord/protocol/KORDItemIDs.h>
#include <kord/protocol/ServerParameters.h>
#include <kord/protocol/ServerServiceIDs.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace kr2::kord {

/**
 * @class Request
 * @brief Base class representing a generic request in the KORD API.
 *
 * This class encapsulates the common properties and behaviors of all requests
 * sent to the KORD system. It includes identifiers, request types, status, and
 * error codes.
 */
class Request {
public:
    /**
     * @brief Unique identifier for the request, based on a nanosecond timestamp.
     */
    int64_t request_rid_{0};

    /**
     * @brief Type of the request, defined by EKORDItemID.
     */
    protocol::EKORDItemID request_type_{};

    /**
     * @brief System-specific request type, defined by EControlCommandItems.
     */
    protocol::EControlCommandItems system_request_type_{};

    /**
     * @brief Status of the request, defined by EControlCommandStatus.
     */
    protocol::EControlCommandStatus request_status_{};

    /**
     * @brief Error code associated with the request, if any.
     */
    unsigned int error_code_{};

    /**
     * @brief Default constructor that initializes the request identifier.
     */
    Request() : request_rid_(std::chrono::steady_clock::now().time_since_epoch().count()) {}

    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~Request() = default;

    /**
     * @brief Converts the request to a system request type.
     *
     * @return Reference to the modified Request object.
     */
    Request &asSystem()
    {
        this->request_type_ = protocol::EKORDItemID::eRequestSystem;
        return *this;
    }
};

/**
 * @class RequestSystem
 * @brief Represents a system-specific request in the KORD API.
 *
 * This class extends the base Request class to handle system-related commands
 * such as transferring logs, enabling server communication, and retrieving
 * version information.
 */
class RequestSystem : public Request {

public:
    /**
     * @brief Default constructor that sets the request type to system.
     */
    RequestSystem() { request_type_ = protocol::EKORDItemID::eRequestSystem; };

    /**
     * @brief Constructs a RequestSystem from a base Request object.
     *
     * @param a_req The base Request object to copy from.
     */
    explicit RequestSystem(const Request &a_req) : Request(a_req){};

    /**
     * @brief Sets the command item to TransferLogFiles and updates the request ID.
     *
     * @return Reference to the modified RequestSystem object.
     */
    RequestSystem &asLogTransfer()
    {
        system_request_type_ = protocol::EControlCommandItems::eTransferLogFiles;
        return *this;
    };

    /**
     * @brief Sets the command item to TransferDashboardJson and updates the request ID.
     *
     * @return Reference to the modified RequestSystem object.
     */
    RequestSystem &asDashboardJsonTransfer()
    {
        system_request_type_ = protocol::EControlCommandItems::eTransferDashboardJson;
        return *this;
    };

    /**
     * @brief Sets the command item to TransferCalibrationData and updates the request ID.
     *
     * @return Reference to the modified RequestSystem object.
     */
    RequestSystem &asCalibrationDataTransfer()
    {
        system_request_type_ = protocol::EControlCommandItems::eTransferCalibrationData;
        return *this;
    };

    /**
     * @brief Enables or disables server communication capabilities of KORD.
     * This command is only available in tabletless mode. The behavior of the robot
     * is undefined if this command is sent while the robot is in tablet mode.
     *
     * Note: Server communication should be DISABLED when running in real-time mode!
     *
     * @param enable True to enable server communication, false to disable.
     * @return Reference to the modified RequestSystem object.
     */
    RequestSystem &asServerCommunication(bool enable)
    {
        if (enable) {
            system_request_type_ = protocol::EControlCommandItems::eServerEnableCommunication;
        }
        else {
            system_request_type_ = protocol::EControlCommandItems::eServerDisableCommunication;
        }
        request_rid_ = std::chrono::steady_clock::now().time_since_epoch().count();
        return *this;
    }

    /**
     * @brief Retrieves the current versions of software, firmware, and hardware.
     *
     * @return Reference to the modified RequestSystem object.
     */
    RequestSystem &asGetVersion()
    {
        system_request_type_ = protocol::EControlCommandItems::eGetVersion;
        request_rid_ = std::chrono::steady_clock::now().time_since_epoch().count();
        return *this;
    }

    /**
     * @brief Retrieves safety zones from the robot.
     *
     * @return Reference to the modified RequestSystem object.
     */
    RequestSystem &asGetSafetyZones()
    {
        system_request_type_ = protocol::EControlCommandItems::eGetSafetyZones;
        request_rid_ = std::chrono::steady_clock::now().time_since_epoch().count();
        return *this;
    }

    /**
     * @brief Retrieves the robot model, tool IO presence, and serial numbers.
     *
     * @return Reference to the modified RequestSystem object.
     */
    RequestSystem &asGetRobotInfo()
    {
        system_request_type_ = protocol::EControlCommandItems::eGetRobotInfo;
        request_rid_ = std::chrono::steady_clock::now().time_since_epoch().count();
        return *this;
    }
};

/**
 * @class RequestTransfer
 * @brief Represents a file transfer request in the KORD API.
 *
 * This class extends the base Request class to handle file transfer commands,
 * including transferring multiple files with specific masks.
 */
class RequestTransfer : public Request {

public:
    /**
     * @brief Bitmask representing the types of files to transfer.
     */
    uint32_t transfer_mask_{0};

    /**
     * @brief Default constructor that sets the request type to transfer.
     */
    RequestTransfer() { request_type_ = protocol::EKORDItemID::eRequestTransfer; };

    /**
     * @brief Constructs a RequestTransfer from a base Request object.
     *
     * @param a_req The base Request object to copy from.
     */
    explicit RequestTransfer(const Request &a_req) : Request(a_req){};

    /**
     * @brief Sets the command item to TransferFiles with a specific mask and updates the request ID.
     *
     * @param a_mask Bitmask indicating which files to transfer.
     * @return Reference to the modified RequestTransfer object.
     */
    RequestTransfer &asMultipleFilesTransfer(uint32_t a_mask)
    {
        system_request_type_ = protocol::EControlCommandItems::eTransferFiles;
        request_rid_ = std::chrono::steady_clock::now().time_since_epoch().count();
        transfer_mask_ = a_mask;
        return *this;
    };

    /**
     * @brief Sets the command item to TransferFiles and updates the request ID.
     *
     * @return Reference to the modified RequestTransfer object.
     */
    RequestTransfer &asFilesTransfer()
    {
        system_request_type_ = protocol::EControlCommandItems::eTransferFiles;
        request_rid_ = std::chrono::steady_clock::now().time_since_epoch().count();
        transfer_mask_ = 0;
        return *this;
    };

    /**
     * @brief Sets a flag to transfer logs during the TransferFiles operation.
     *
     * @return Reference to the modified RequestTransfer object.
     */
    RequestTransfer &withLogs()
    {
        transfer_mask_ |= 1 << 0;
        return *this;
    };

    /**
     * @brief Sets a flag to transfer dashboard JSON data during the TransferFiles operation.
     *
     * @return Reference to the modified RequestTransfer object.
     */
    RequestTransfer &withDashboardJson()
    {
        transfer_mask_ |= 1 << 1;
        return *this;
    };

    /**
     * @brief Sets a flag to transfer calibration data during the TransferFiles operation.
     *
     * @return Reference to the modified RequestTransfer object.
     */
    RequestTransfer &withCalibration()
    {
        transfer_mask_ |= 1 << 2;
        return *this;
    };
};

/**
 * @class RequestRCAPICommand
 * @brief Represents a Remote Control API command request in the KORD API.
 *
 * This class extends the base Request class to handle RCAPI-specific commands,
 * including user consent and payload management.
 */
class RequestRCAPICommand : public Request {
public:
    /**
     * @brief Identifier for the specific RCAPI command.
     */
    uint16_t command_id_{0};

    /**
     * @brief Length of the payload data.
     */
    uint32_t payload_length_{0};

    /**
     * @brief Payload data associated with the RCAPI command.
     */
    std::vector<uint8_t> payload_{{}};

    /**
     * @brief Default constructor that sets the request type to RCAPI command.
     */
    RequestRCAPICommand() { request_type_ = protocol::EKORDItemID::eCommandRCAPI; };

    /**
     * @brief Constructs a RequestRCAPICommand from a base Request object.
     *
     * @param a_req The base Request object to copy from.
     */
    explicit RequestRCAPICommand(const Request &a_req) : Request(a_req){};

    /**
     * @brief Sets the command to User Consent.
     *
     * @return Reference to the modified RequestRCAPICommand object.
     */
    RequestRCAPICommand &asUserConsent()
    {
        system_request_type_ = protocol::EControlCommandItems::eRCAPICommand;
        command_id_ = protocol::ERCAPICommandIds::eUserConsent;
        payload_ = {};
        return *this;
    };

    /**
     * @brief Adds a byte to the payload.
     *
     * @param byte The byte to add to the payload.
     * @return Reference to the modified RequestRCAPICommand object.
     */
    RequestRCAPICommand &addPayload(uint8_t byte)
    {
        payload_length_ += 1;
        payload_.push_back(byte);
        return *this;
    };
};

/**
 * @class RequestServer
 * @brief Represents a server request in the KORD API.
 *
 * This class extends the base Request class to handle server-specific commands,
 * including service requests with optional parameters.
 *
 * @note The robot must operate in tabletless mode to handle server requests.
 */
class RequestServer : public Request {
public:
    /**
     * @brief Default constructor that sets the request type to server.
     */
    RequestServer() { request_type_ = protocol::EKORDItemID::eRequestServer; }

    /**
     * @brief Constructs a RequestServer from a base Request object.
     *
     * @param a_req The base Request object to copy from.
     */
    explicit RequestServer(const Request &a_req) : Request(a_req){};

    /**
     * @brief Configures the request as a service request without additional parameters.
     *
     * @param a_command Action to be applied to the service.
     * @param a_service_id Identifier of the service to which the command is applied.
     * @return Reference to the modified RequestServer object.
     */
    RequestServer &asServiceRequest(protocol::EServerServiceCommands a_command,
                                    protocol::EKORDServerServiceID a_service_id)
    {
        request_rid_ = std::chrono::steady_clock::now().time_since_epoch().count();
        system_request_type_ = protocol::EControlCommandItems::eServerRequest;
        command_ = static_cast<uint16_t>(a_command);
        service_id_ = static_cast<uint16_t>(a_service_id);
        parameters_ = nullptr;
        return *this;
    }

    /**
     * @brief Configures the request as a service request with additional parameters.
     *
     * @param a_command Action to be applied to the service. Currently only eStart, eStop and eGetStatus are supported.
     * @param a_service_id Identifier of the service to which the command is applied. Currently only eFetchKincalData is supported.
     * @param a_parameters Shared pointer to the service parameters. The parameters are specific to the service.
     * @note The parameters are optional and can be set to nullptr.
     * @return Reference to the modified RequestServer object.
     */
    RequestServer &asServiceRequest(protocol::EServerServiceCommands a_command,
                                    protocol::EKORDServerServiceID a_service_id,
                                    std::shared_ptr<protocol::ServiceParameters> a_parameters)
    {
        request_rid_ = std::chrono::steady_clock::now().time_since_epoch().count();
        system_request_type_ = protocol::EControlCommandItems::eServerRequest;
        command_ = static_cast<uint16_t>(a_command);
        service_id_ = static_cast<uint16_t>(a_service_id);
        parameters_ = a_parameters;
        return *this;
    }

    /**
     * @brief Command identifier for the server request.
     */
    uint16_t command_{};

    /**
     * @brief Service identifier for the server request.
     */
    uint16_t service_id_{};

    /**
     * @brief Optional parameters for the server request.
     */
    std::shared_ptr<protocol::ServiceParameters> parameters_;
};

} // namespace kr2::kord

#endif // KR2_KORD_API_REQUEST_H
