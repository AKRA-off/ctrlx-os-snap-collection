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

#include <kord/protocol/StatusFrameBuilder.h>
#include <kord/protocol/KORDFrames.h>

using namespace kr2::kord::protocol;

class StatusFrameBuilder::StatusFrameBuilderImpl {
public:
    StatusFrameBuilderImpl()
    {
        reset();
        seq_number_ = 0;
    }

    void reset() { status_frame_.reset(); }

    unsigned int frequency_{};
    uint16_t seq_number_;
    KORDStatusFrame status_frame_{};
};

StatusFrameBuilder::StatusFrameBuilder() : impl_(new StatusFrameBuilderImpl)
{
    StatusFrameBuilder::clear();
    impl_->frequency_ = 0;
}

StatusFrameBuilder::StatusFrameBuilder(unsigned int a_frequency) : impl_(new StatusFrameBuilderImpl)
{
    StatusFrameBuilder::clear();
    impl_->frequency_ = a_frequency;
}

StatusFrameBuilder::StatusFrameBuilder(const StatusFrameBuilder &a_other)
    : ContentWriter(a_other), impl_(new StatusFrameBuilderImpl)
{
    *impl_ = *a_other.impl_;
}

StatusFrameBuilder::~StatusFrameBuilder() { delete impl_; }

StatusFrameBuilder &StatusFrameBuilder::operator=(const StatusFrameBuilder &a_other)
{
    if (&a_other == this) return *this;
    *impl_ = *a_other.impl_;
    return *this;
}

bool StatusFrameBuilder::getPayload(uint8_t *a_buffer, size_t a_buffer_reserve) const
{
    size_t len = impl_->status_frame_.getFrameLength();
    if (a_buffer_reserve < len)
        return false;

    impl_->status_frame_.frequency_ = impl_->frequency_;
    impl_->status_frame_.version_ = static_cast<uint16_t>(kr2::kord::protocol::KORD_STATUS_VERSION_1);
    memcpy(a_buffer, reinterpret_cast<const uint8_t *>(&impl_->status_frame_), len);
    return true;
}

bool StatusFrameBuilder::addSequenceNumber(uint16_t a_sequence_number)
{
    impl_->status_frame_.sequence_number_ = a_sequence_number;
    return true;
}

void StatusFrameBuilder::clear()
{
    impl_->reset();
    this->reset(impl_->status_frame_.data_, impl_->status_frame_.getDataLength());
}
