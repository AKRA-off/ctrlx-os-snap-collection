/*////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright 2022 by Kassow Robots, ApS
//
// The information contained herein is confidential, proprietary to Kassow Robots,
// ApS, and considered a trade secret as defined in section 263 and section 264
// under the Danish Criminal Code. Use of this information by anyone
// other than authorized employees of Kassow Robots, ApS is granted only under a
// written non-disclosure agreement, expressly prescribing the scope and
// manner of such use.
//
// Authors: Alexander Kazakov aka@kassowrobots.com
//
/////////////////////////////////////////////////////////////////////////////*/

#include <kord/api/kord_io_request.h>

#include <stdexcept>

namespace kr2::kord {

using namespace kr2::kord;

RequestIO::RequestIO() { request_type_ = protocol::EKORDItemID::eCommandSetIODigitalOut; }

RequestIO::RequestIO(const Request &a_req) : Request(a_req){}

RequestIO &RequestIO::asSetIODigitalOut()
{
    this->system_request_type_ = protocol::EControlCommandItems::eIOSet;
    this->request_rid_ = std::chrono::system_clock::now().time_since_epoch().count();
    this->request_type_ = protocol::EKORDItemID::eCommandSetIODigitalOut;
    return *this;
}

// RequestIO& RequestIO::asSetIOAnalogOut()
// {
//     this->system_request_type_ = kkp::EControlCommandItems::eIOSet;
//     this->request_rid_         = std::chrono::high_resolution_clock::now().time_since_epoch().count();
//     this->request_type_        = kkp::EKORDItemID::eCommandSetIOAnalogOut;
//     return *this;
// }

RequestIO &RequestIO::withEnabledPorts(int64_t mask)
{
    this->value_ = 1;
    this->mask_ = (int64_t)mask;
    return *this;
}

RequestIO &RequestIO::withDisabledPorts(int64_t mask)
{
    this->value_ = 0;
    this->mask_ = (int64_t)mask;
    return *this;
}

RequestIO &RequestIO::withEnabledSafePorts(int64_t mask, ESafePortConfiguration config)
{
    if ((mask & (MASK_SDO1 | MASK_SDO2 | MASK_SDO3 | MASK_SDO4)) == 0) {
        throw std::invalid_argument("Specified ports are not used for safety mapping.");
    }

    this->value_ = 0;
    this->mask_ = (int64_t)mask;
    this->config_id_ = config;
    return *this;
}

} // namespace kr2::kord
