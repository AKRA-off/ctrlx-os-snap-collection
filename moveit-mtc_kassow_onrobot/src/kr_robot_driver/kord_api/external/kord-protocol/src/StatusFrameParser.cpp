/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2022, Kassow Robots
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

#include <kord/protocol/StatusFrameParser.h>
#include <kord/protocol/KORDFrames.h>

using namespace kr2::kord::protocol;

class StatusFrameParser::StatusFrameParserImpl {
public:
    StatusFrameParserImpl() { reset(); }

    void reset() { status_frame_.reset(); }

    KORDStatusFrame status_frame_{};
};

StatusFrameParser::StatusFrameParser() : impl_(new StatusFrameParserImpl)
{
    this->reset(impl_->status_frame_.data_, impl_->status_frame_.getDataLength());
}

StatusFrameParser::StatusFrameParser(const StatusFrameParser &a_other)
    : ContentReader(a_other), impl_(new StatusFrameParserImpl)
{
    *impl_ = *a_other.impl_;
}

StatusFrameParser::~StatusFrameParser() { delete impl_; }

StatusFrameParser &StatusFrameParser::operator=(const StatusFrameParser &a_other)
{
    *impl_ = *a_other.impl_;
    return *this;
}

bool StatusFrameParser::setFromPayload(uint8_t *data, size_t data_len)
{
    if (data_len > impl_->status_frame_.getFrameLength())
        return false;

    impl_->status_frame_.reset();

    memcpy(reinterpret_cast<uint8_t *>(&impl_->status_frame_), data, data_len);

    return true;
}

unsigned int StatusFrameParser::getFrequency() const { return impl_->status_frame_.frequency_; }

unsigned int StatusFrameParser::getVersion() const { return impl_->status_frame_.version_; }

unsigned int StatusFrameParser::getSequenceNumber() const { return impl_->status_frame_.sequence_number_; }

void StatusFrameParser::clear()
{
    impl_->reset();
    this->reset(impl_->status_frame_.data_, impl_->status_frame_.getDataLength());
}
