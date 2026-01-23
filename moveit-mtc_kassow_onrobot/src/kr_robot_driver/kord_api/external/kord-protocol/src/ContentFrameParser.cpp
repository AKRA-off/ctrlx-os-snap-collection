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

#include <kord/protocol/ContentFrameParser.h>
#include <kord/protocol/ContentItem.h>
#include <kord/protocol/KORDFrames.h>

#include <iostream>

using namespace kr2::kord::protocol;

template <typename ContentFrameType> class ContentFrameParser<ContentFrameType>::ContentFrameParserImpl {
public:
    ContentFrameParserImpl() = default;

    KORDContentHeader *getHeader(int a_idx)
    {
        if ((a_idx >= MAX_CONTENT_ITEMS) || (a_idx < 0))
            return nullptr;

        auto *hdr = reinterpret_cast<KORDContentHeader *>(content_frame_.item_headers_);
        hdr += a_idx;
        return hdr;
    }

    void reset() { content_frame_.reset(); }

    ContentFrameType content_frame_;
};

template <typename ContentFrameType>
ContentFrameParser<ContentFrameType>::ContentFrameParser() : impl_(new ContentFrameParserImpl)
{
}

template <typename ContentFrameType>
ContentFrameParser<ContentFrameType>::ContentFrameParser(const ContentFrameParser &a_other)
    : impl_(new ContentFrameParserImpl)
{
    *impl_ = *a_other.impl_;
}

template <typename ContentFrameType> ContentFrameParser<ContentFrameType>::~ContentFrameParser() = default;

template <typename ContentFrameType>
bool ContentFrameParser<ContentFrameType>::setFromPayload(const uint8_t *a_payload, size_t a_dlen)
{
    size_t cf_len = impl_->content_frame_.getFrameLength();
    if (!a_payload) {
        std::cout << "Error: Payload is null" << std::endl;
        return false;
    }

    if (a_dlen > cf_len) {
        std::cerr << "Error: Payload length is greater than frame length" << std::endl;
        return false;
    }

    std::memcpy(&impl_->content_frame_, a_payload, cf_len);
    return true;
}

template <typename ContentFrameType> int ContentFrameParser<ContentFrameType>::getItemsCount() const
{
    return impl_->content_frame_.items_contained_;
}

template <typename ContentFrameType> EKORDItemID ContentFrameParser<ContentFrameType>::getItemID(int a_idx) const
{
    KORDContentHeader *hdr = impl_->getHeader(a_idx);

    if (!hdr) {
        std::cerr << "Error: Header is null" << std::endl;
        return EKORDItemID::eNone;
    }

    return static_cast<EKORDItemID>((hdr)->item_id_);
}

template <typename ContentFrameType>
ContentItem<ContentFrameParser<ContentFrameType>::ContentFrameSize> ContentFrameParser<ContentFrameType>::getItemContent(
    int a_idx) const
{
    KORDContentHeader *hdr = impl_->getHeader(a_idx);

    if (hdr == nullptr) {
        return {EKORDItemID::eNone, nullptr, 0};
    }

    return {static_cast<EKORDItemID>(hdr->item_id_),
            impl_->content_frame_.item_data_ + hdr->item_offset_,
            hdr->item_length_};
}

template <typename ContentFrameType>
ContentFrameParser<ContentFrameType> &ContentFrameParser<ContentFrameType>::operator=(const ContentFrameParser &a_other)
{
    if (this == &a_other) {
        return *this;
    }

    *impl_ = *a_other.impl_;
    return *this;
}

template <typename ContentFrameType> bool ContentFrameParser<ContentFrameType>::clear()
{
    impl_->reset();
    return true;
}

template class kr2::kord::protocol::ContentFrameParser<KORDContentFrame>;
template class kr2::kord::protocol::ContentFrameParser<KORDContentFrameTCP>;
