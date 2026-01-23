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

#ifndef KR2_KORD_API_RESPONSE_CODES_H
#define KR2_KORD_API_RESPONSE_CODES_H

#include <cstdint>

namespace kr2::kord::protocol {

/**
 * @brief Command status flag represents in what state command has ended up.
 *
 */
enum class EResponseCodes : uint8_t {
    // Success Codes
    SUCCESS = 0,                       // General success

    // Client Error Codes
    ERROR_BAD_REQUEST = 10,            // The request was invalid or cannot be otherwise served
    ERROR_NOT_FOUND = 11,              // The requested resource could not be found
    ERROR_METHOD_NOT_ALLOWED = 12,     // A request was made of a resource using a request method not supported by that resource

    // Server Error Codes
    ERROR_INTERNAL_SERVER = 20,        // A generic error message, given when no more specific message is suitable
    ERROR_NOT_IMPLEMENTED = 21,        // The server either does not recognize the request method, or it lacks the ability to fulfill the request
    ERROR_VERSION_NOT_SUPPORTED = 22,  // The server does not support the protocol version used in the request

    // Versioning and Compatibility
    ERROR_VERSION_MISMATCH = 30,       // Client and server version mismatch
    ERROR_DEPRECATED_API = 31,         // Deprecated API usage

    ERROR = 254                        // General error
};


} // namespace kr2::kord::protocol
#endif //KR2_KORD_API_RESPONSE_CODES_H
