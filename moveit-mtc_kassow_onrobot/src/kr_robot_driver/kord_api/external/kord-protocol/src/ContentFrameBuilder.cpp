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

#include <kord/protocol/ContentFrameBuilder.h>
#include <kord/protocol/KORDFrames.h>

#include <iostream>

using namespace kr2::kord::protocol;

template <typename ContentFrameType> class ContentFrameBuilder<ContentFrameType>::ContentFrameBuilderImpl {
public:
    ContentFrameBuilderImpl() { content_frame_.reset(); }

    void reset()
    {
        n_content_items_ = 0;
        content_frame_.reset();
        item_start_ = content_frame_.item_data_;
        for (auto &hdr : headers_) {
            hdr.reset();
        }
    }

    bool addItem(EKORDItemID a_iid, const uint8_t *a_data, size_t a_content_length)
    {
        ptrdiff_t item_offset = item_start_ - content_frame_.item_data_;
        ptrdiff_t end_item_offset = item_offset + a_content_length + 2; // increment by two for the end data marker

        if (end_item_offset < 0 || static_cast<size_t>(end_item_offset) > content_frame_.getDataLength()) {
            return false;
        }

        headers_[n_content_items_].item_id_ = static_cast<uint32_t>(a_iid);
        headers_[n_content_items_].item_offset_ = static_cast<uint16_t>(item_offset);
        headers_[n_content_items_].item_length_ = static_cast<uint16_t>(a_content_length);
        memcpy(item_start_, a_data, a_content_length);
        item_start_ += a_content_length;
        // add end data marker 0xff00
        *item_start_ = 0xff;
        item_start_++;
        *item_start_ = 0x00;
        item_start_++;
        n_content_items_++;

        return true;
    }

    void nextItem() { n_content_items_++; }

    [[nodiscard]] bool itemsFull() const { return (n_content_items_ >= MAX_CONTENT_ITEMS); }

    ContentFrameType content_frame_{};
    uint16_t n_content_items_{};
    std::array<KORDContentHeader, MAX_CONTENT_ITEMS> headers_{};
    uint8_t *item_start_{};
};

template <typename ContentFrameType>
ContentFrameBuilder<ContentFrameType>::ContentFrameBuilder() : impl_(new ContentFrameBuilderImpl)
{
    this->clear();
}

template <typename ContentFrameType>
ContentFrameBuilder<ContentFrameType>::ContentFrameBuilder(const ContentFrameBuilder &a_other)
    : impl_(new ContentFrameBuilderImpl)
{
    *impl_ = *a_other.impl_;
}

template <typename ContentFrameType> ContentFrameBuilder<ContentFrameType>::~ContentFrameBuilder() { delete impl_; }

template <typename ContentFrameType>
ContentFrameBuilder<ContentFrameType> &ContentFrameBuilder<ContentFrameType>::operator=(
    const ContentFrameBuilder &a_other)
{
    *impl_ = *a_other.impl_;
    return *this;
}

template <typename ContentFrameType>
bool ContentFrameBuilder<ContentFrameType>::getPayload(uint8_t *a_buffer, size_t a_buffer_reserve)
{
    size_t len = impl_->content_frame_.getFrameLength();

    if (len > a_buffer_reserve)
        return false;

    impl_->content_frame_.items_contained_ = impl_->n_content_items_;
    memcpy(impl_->content_frame_.item_headers_, impl_->headers_.data(), sizeof(impl_->headers_));
    memcpy(a_buffer, &impl_->content_frame_, len);

    return true;
}

template <typename ContentFrameType>
bool ContentFrameBuilder<ContentFrameType>::addContentItem(EKORDItemID a_iid, uint8_t *a_data, size_t a_content_length)
{
    if (impl_->itemsFull()) {
        return false;
    }

    if (!impl_->addItem(a_iid, a_data, a_content_length)) {
        return false;
    };

    return true;
}

template <typename ContentFrameType>
bool ContentFrameBuilder<ContentFrameType>::addContentItem(const ContentItem<ContentFrameSize> &a_item)
{
    if (impl_->itemsFull()) {
        return false;
    }

    if (!impl_->addItem(a_item.getItemID(), a_item.getItemData(), a_item.getItemDataLength())) {
        return false;
    };

    return true;
}

template <typename ContentFrameType> bool ContentFrameBuilder<ContentFrameType>::clear()
{
    impl_->reset();
    return true;
}

template class kr2::kord::protocol::ContentFrameBuilder<KORDContentFrame>;
template class kr2::kord::protocol::ContentFrameBuilder<KORDContentFrameTCP>;
